/*
 * Copyright 2014 Charles University in Prague
 * Copyright 2014 Vojtech Horky
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define  _POSIX_C_SOURCE 200809L

// TODO: reintroduce conditional compilation if getrusage or PAPI
// is not available

#define NOTSUP_LONG ((long)-1)


#include "cz_cuni_mff_d3s_perf_CompilationCounter.h"
#include "microbench.h"
#include "metric.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/resource.h>
#include <papi.h>

typedef struct timespec timestamp_t;
#define PRI_TIMESTAMP_FMT "%6ld.%09ld"
#define PRI_TIMESTAMP(x) (x).tv_sec, (x).tv_nsec

static inline long long timestamp_diff_ns(const timestamp_t *a, const timestamp_t *b) {
	long long sec_diff = b->tv_sec - a->tv_sec;
	long long nanosec_diff = b->tv_nsec - a->tv_nsec;
	return sec_diff * 1000 * 1000 * 1000 + nanosec_diff;
}

typedef struct {
	timestamp_t timestamp;
	struct rusage resource_usage;
	long compilations;
	int garbage_collections;
	long long papi_events[UBENCH_EVENT_COUNT];
} ubench_events_snapshot_t;


typedef struct {
	ubench_events_snapshot_t start;
	ubench_events_snapshot_t end;
} benchmark_run_t;


#define UBENCH_EVENT_BACKEND_LINUX 1
#define UBENCH_EVENT_BACKEND_RESOURCE_USAGE 2
#define UBENCH_EVENT_BACKEND_PAPI 4

#define ubench_event_info_INIT(m_index, m_shortname, m_backend, m_getter, m_papi_event_id) \
	do { \
		ubench_event_info[m_index].id = #m_index; \
		ubench_event_info[m_index].short_name = m_shortname; \
		ubench_event_info[m_index].backend = m_backend; \
		ubench_event_info[m_index].papi_event_id = m_papi_event_id; \
		ubench_event_info[m_index].op_get = m_getter; \
	} while (0)


typedef struct ubench_event_info ubench_event_info_t;
typedef long long (*event_getter_func_t)(const benchmark_run_t *, const ubench_event_info_t *);
struct ubench_event_info {
	const char *id;
	const char *short_name;
	unsigned int backend;
	int papi_event_id;
	event_getter_func_t op_get;
};

static ubench_event_info_t ubench_event_info[UBENCH_EVENT_COUNT];
static unsigned int backends_used;
static int papi_counters[UBENCH_EVENT_COUNT];
static size_t papi_counters_count;
static int *used_events_indices = NULL;
static size_t used_events_count = 0;


static benchmark_run_t *benchmark_runs = NULL;
static size_t benchmark_runs_size = 0;
static size_t benchmark_runs_index = 0;



static inline void store_current_timestamp(timestamp_t *ts) {
	clock_gettime(CLOCK_MONOTONIC, ts);
}

static long long getter_wall_clock_time(const benchmark_run_t *bench, const ubench_event_info_t *info) {
	return timestamp_diff_ns(&bench->start.timestamp, &bench->end.timestamp);
}

static long long getter_context_switch_forced(const benchmark_run_t *bench, const ubench_event_info_t *info) {
	return bench->end.resource_usage.ru_nivcsw - bench->start.resource_usage.ru_nivcsw;
}

static long long getter_papi(const benchmark_run_t *bench, const ubench_event_info_t *info) {
	int papi_event = info->papi_event_id;
	size_t index = 0;
	while (index < papi_counters_count) {
		if (papi_counters[index] == papi_event) {
			break;
		}
	}
	if (index >= papi_counters_count) {
		return -1;
	}
	return bench->end.papi_events[index] - bench->start.papi_events[index];
}

jint ubench_benchmark_init(void) {
	// TODO: check for errors
	PAPI_library_init(PAPI_VER_CURRENT);

	ubench_event_info_INIT(UBENCH_EVENT_WALL_CLOCK_TIME, "clock", UBENCH_EVENT_BACKEND_LINUX, getter_wall_clock_time, -1);
	ubench_event_info_INIT(UBENCH_EVENT_CONTEXT_SWITCH_FORCED, "ctxsw", UBENCH_EVENT_BACKEND_RESOURCE_USAGE, getter_context_switch_forced, -1);
	ubench_event_info_INIT(UBENCH_EVENT_L2_DATA_READ, "l2dr", UBENCH_EVENT_BACKEND_PAPI, getter_papi, PAPI_L2_DCR);

	return JNI_OK;
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_init(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass),
		jint jmeasurements, jintArray jevents) {
	free(benchmark_runs);
	free(used_events_indices);

	size_t measurement_count = jmeasurements;

	int events_count = (*env)->GetArrayLength(env, jevents);
	if (events_count == 0) {
		return;
	}

	used_events_indices = malloc(sizeof(int) * events_count);

	jint *events = (*env)->GetIntArrayElements(env, jevents, NULL);
	backends_used = 0;

	papi_counters_count = 0;
	used_events_count = 0;
	for (int i = 0; i < events_count; i++) {
		used_events_indices[i] = events[i];
		int metric_id = used_events_indices[i];
		if ((metric_id < 0) || (metric_id >= UBENCH_EVENT_COUNT)) {
			continue;
		}
		used_events_count++;
		unsigned int backend = ubench_event_info[metric_id].backend;
		if (backend == UBENCH_EVENT_BACKEND_PAPI) {
			if (papi_counters_count < UBENCH_EVENT_COUNT) {
				/* Check that the event is not already registered. */
				papi_counters[papi_counters_count] = ubench_event_info[metric_id].papi_event_id;
				papi_counters_count++;
			}
		}

		backends_used |= backend;
	}
	(*env)->ReleaseIntArrayElements(env, jevents, events, JNI_ABORT);

	benchmark_runs = malloc(sizeof(benchmark_run_t) * measurement_count);
	benchmark_runs_size = measurement_count;
	benchmark_runs_index = 0;
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_start(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	if (benchmark_runs_index >= benchmark_runs_size) {
		benchmark_runs_index = benchmark_runs_size - 1;
	}

	getrusage(RUSAGE_SELF, &benchmark_runs[benchmark_runs_index].start.resource_usage);
	benchmark_runs[benchmark_runs_index].start.compilations = ubench_atomic_get(&counter_compilation_total);
	benchmark_runs[benchmark_runs_index].start.garbage_collections = ubench_atomic_get(&counter_gc_total);

	// TODO: check for errors
	PAPI_start_counters(papi_counters, papi_counters_count);
	PAPI_read_counters(benchmark_runs[benchmark_runs_index].start.papi_events, papi_counters_count);

	store_current_timestamp(&benchmark_runs[benchmark_runs_index].start.timestamp);
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_stop(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	store_current_timestamp(&benchmark_runs[benchmark_runs_index].end.timestamp);

	// TODO: check for errors
	PAPI_stop_counters(benchmark_runs[benchmark_runs_index].end.papi_events, papi_counters_count);

	benchmark_runs[benchmark_runs_index].end.garbage_collections = ubench_atomic_get(&counter_gc_total);
	benchmark_runs[benchmark_runs_index].end.compilations = ubench_atomic_get(&counter_compilation_total);

	getrusage(RUSAGE_SELF, &benchmark_runs[benchmark_runs_index].end.resource_usage);

	benchmark_runs_index++;
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_dump(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass),
		jstring jfilename) {
	const char *filename = (*env)->GetStringUTFChars(env, jfilename, 0);

	FILE *file = NULL;
	if (strcmp(filename, "-") == 0) {
		file = stdout;
	} else {
		file = fopen(filename, "wt");
	}

	if (file == NULL) {
		goto leave;
	}

	/* Print the results. */
	for (size_t bi = 0; bi < benchmark_runs_index; bi++) {
		for (size_t fi = 0; fi < used_events_count; fi++) {
			ubench_event_info_t *info = &ubench_event_info[ used_events_indices[fi] ];
			long long value = info->op_get(&benchmark_runs[bi], info);
			fprintf(file, "%12lld", value);
		}
		fprintf(file, "\n");
	}

	if (file != stdout) {
		fclose(file);
	}

leave:
	(*env)->ReleaseStringUTFChars(env, jfilename, filename);
}
