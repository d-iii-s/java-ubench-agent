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
#include <stddef.h>
#include <time.h>
#include <string.h>
#include <sys/resource.h>
#include <papi.h>

typedef struct timespec timestamp_t;
#define PRI_TIMESTAMP_FMT "%6ld.%09ld"
#define PRI_TIMESTAMP(x) (x).tv_sec, (x).tv_nsec


#define UBENCH_EVENT_BACKEND_LINUX 1
#define UBENCH_EVENT_BACKEND_RESOURCE_USAGE 2
#define UBENCH_EVENT_BACKEND_PAPI 4


typedef struct {
	timestamp_t timestamp;
	struct rusage resource_usage;
	long compilations;
	int garbage_collections;
	long long papi_events[UBENCH_EVENT_COUNT];
	int papi_rc1;
	int papi_rc2;
} ubench_events_snapshot_t;

typedef struct {
	ubench_events_snapshot_t start;
	ubench_events_snapshot_t end;
} benchmark_run_t;


typedef struct {
	unsigned int used_backends;

	int used_events[UBENCH_EVENT_COUNT];
	size_t used_events_count;

	int used_papi_events[UBENCH_EVENT_COUNT];
	size_t used_papi_events_count;

	benchmark_run_t *data;
	size_t data_size;
	size_t data_index;
} benchmark_configuration_t;


typedef struct ubench_event_info ubench_event_info_t;
typedef long long (*event_getter_func_t)(const benchmark_run_t *, const ubench_event_info_t *);
struct ubench_event_info {
	const char *id;
	const char *short_name;
	unsigned int backend;
	int papi_event_id;
	event_getter_func_t op_get;
};

#define UBENCH_EVENT_INFO_INIT(m_index, m_shortname, m_backend, m_getter, m_papi_event_id) \
	do { \
		all_known_events_info[m_index].id = #m_index; \
		all_known_events_info[m_index].short_name = m_shortname; \
		all_known_events_info[m_index].backend = m_backend; \
		all_known_events_info[m_index].papi_event_id = m_papi_event_id; \
		all_known_events_info[m_index].op_get = m_getter; \
	} while (0)

static ubench_event_info_t all_known_events_info[UBENCH_EVENT_COUNT];

static benchmark_configuration_t current_benchmark;


static inline long long timestamp_diff_ns(const timestamp_t *a, const timestamp_t *b) {
	long long sec_diff = b->tv_sec - a->tv_sec;
	long long nanosec_diff = b->tv_nsec - a->tv_nsec;
	return sec_diff * 1000 * 1000 * 1000 + nanosec_diff;
}

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
	if (bench->start.papi_rc1 != PAPI_OK) {
		return -1;
	} else if (bench->start.papi_rc2 != PAPI_OK) {
		return -1;
	} else if (bench->end.papi_rc1 != PAPI_OK) {
		return -1;
	}
	int papi_event = info->papi_event_id;

	size_t index = 0;
	while (index < current_benchmark.used_papi_events_count) {
		if (current_benchmark.used_papi_events[index] == papi_event) {
			break;
		}
		index++;
	}
	if (index >= current_benchmark.used_papi_events_count) {
		return -1;
	}

	return bench->end.papi_events[index] - bench->start.papi_events[index];
}

jint ubench_benchmark_init(void) {
	// TODO: check for errors
	PAPI_library_init(PAPI_VER_CURRENT);

	UBENCH_EVENT_INFO_INIT(UBENCH_EVENT_WALL_CLOCK_TIME, "clock", UBENCH_EVENT_BACKEND_LINUX, getter_wall_clock_time, -1);
	UBENCH_EVENT_INFO_INIT(UBENCH_EVENT_CONTEXT_SWITCH_FORCED, "ctxsw", UBENCH_EVENT_BACKEND_RESOURCE_USAGE, getter_context_switch_forced, -1);

	UBENCH_EVENT_INFO_INIT(UBENCH_EVENT_L2_DATA_READ, "l2dr", UBENCH_EVENT_BACKEND_PAPI, getter_papi, PAPI_L2_DCR);

	UBENCH_EVENT_INFO_INIT(UBENCH_EVENT_L1_CACHE_DATA_MISS, "l1dcm", UBENCH_EVENT_BACKEND_PAPI, getter_papi, PAPI_L1_DCM);
	UBENCH_EVENT_INFO_INIT(UBENCH_EVENT_L2_CACHE_DATA_MISS, "l2dcm", UBENCH_EVENT_BACKEND_PAPI, getter_papi, PAPI_L2_DCM);
	UBENCH_EVENT_INFO_INIT(UBENCH_EVENT_L3_CACHE_DATA_MISS, "l3dcm", UBENCH_EVENT_BACKEND_PAPI, getter_papi, PAPI_L2_DCM);

	current_benchmark.used_backends = 0;
	current_benchmark.used_events_count = 0;
	current_benchmark.used_papi_events_count = 0;

	current_benchmark.data = NULL;
	current_benchmark.data_index = 0;
	current_benchmark.data_size = 0;

	return JNI_OK;
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_init(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass),
		jint jmeasurements, jintArray jevents) {
	free(current_benchmark.data);
	current_benchmark.data = NULL;
	current_benchmark.data_index = 0;
	current_benchmark.data_size = 0;

	current_benchmark.used_backends = 0;
	current_benchmark.used_events_count = 0;
	current_benchmark.used_papi_events_count = 0;

	size_t events_count = (*env)->GetArrayLength(env, jevents);
	if (events_count == 0) {
		return;
	}

	if (jmeasurements <= 0) {
		return;
	}

	current_benchmark.data = malloc(sizeof(benchmark_run_t) * jmeasurements);
	current_benchmark.data_size = jmeasurements;

	jint *events = (*env)->GetIntArrayElements(env, jevents, NULL);

	for (size_t i = 0; i < events_count; i++) {
		int event_id = events[i];
		if ((event_id < 0) || (event_id >= UBENCH_EVENT_COUNT)) {
			continue;
		}
		// Silently drop duplicates
		int is_duplicate = 0;
		for (size_t j = 0; j < current_benchmark.used_events_count; j++) {
			if (current_benchmark.used_events[j] == event_id) {
				is_duplicate = 1;
				break;
			}
		}
		if (is_duplicate) {
			continue;
		}

		current_benchmark.used_events[current_benchmark.used_events_count] = event_id;
		current_benchmark.used_events_count++;

		ubench_event_info_t *event_info = &all_known_events_info[event_id];

		current_benchmark.used_backends |= event_info->backend;

		if (event_info->backend == UBENCH_EVENT_BACKEND_PAPI) {
			current_benchmark.used_papi_events[current_benchmark.used_papi_events_count] = event_info->papi_event_id;
			current_benchmark.used_papi_events_count++;
		}
	}

#if 0
	for (size_t i = 0; i < current_benchmark.used_events_count; i++) {
		int event_id = current_benchmark.used_events[i];
		fprintf(stderr, "Event #%2zu: %s\n", i, all_known_events_info[event_id].id);
	}
#endif

	(*env)->ReleaseIntArrayElements(env, jevents, events, JNI_ABORT);
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_start(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	if (current_benchmark.data_size == 0) {
		return;
	}
	if (current_benchmark.data_index >= current_benchmark.data_size) {
		current_benchmark.data_index--;
	}

	ubench_events_snapshot_t *snapshot = &current_benchmark.data[current_benchmark.data_index].start;

	getrusage(RUSAGE_SELF, &(snapshot->resource_usage));
	snapshot->compilations = ubench_atomic_get(&counter_compilation_total);
	snapshot->garbage_collections = ubench_atomic_get(&counter_gc_total);

	// TODO: check for errors
	snapshot->papi_rc1 = PAPI_start_counters(current_benchmark.used_papi_events, current_benchmark.used_papi_events_count);
	snapshot->papi_rc2 = PAPI_read_counters(snapshot->papi_events, current_benchmark.used_papi_events_count);

	store_current_timestamp(&(snapshot->timestamp));
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_stop(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	ubench_events_snapshot_t *snapshot = &current_benchmark.data[current_benchmark.data_index].end;

	store_current_timestamp(&(snapshot->timestamp));

	// TODO: check for errors
	snapshot->papi_rc1 = PAPI_stop_counters(snapshot->papi_events, current_benchmark.used_papi_events_count);

	snapshot->compilations = ubench_atomic_get(&counter_compilation_total);
	snapshot->garbage_collections = ubench_atomic_get(&counter_gc_total);

	getrusage(RUSAGE_SELF, &(snapshot->resource_usage));

	current_benchmark.data_index++;
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
	for (size_t bi = 0; bi < current_benchmark.data_index; bi++) {
		benchmark_run_t *benchmark = &current_benchmark.data[bi];

		for (size_t ei = 0; ei < current_benchmark.used_events_count; ei++) {
			int event_id = current_benchmark.used_events[ei];
			ubench_event_info_t *info = &all_known_events_info[event_id];
			long long value = info->op_get(benchmark, info);
			fprintf(file, "%12lld", value);
		}

		if (current_benchmark.used_backends & UBENCH_EVENT_BACKEND_PAPI) {
			if (benchmark->start.papi_rc1 != PAPI_OK) {
				fprintf(file, " start_counters=%s", PAPI_strerror(benchmark->start.papi_rc1));
			} else if (benchmark->start.papi_rc2 != PAPI_OK) {
				fprintf(file, " read_counters=%s", PAPI_strerror(benchmark->start.papi_rc2));
			} else if (benchmark->end.papi_rc1 != PAPI_OK) {
				fprintf(file, " stop_counters=%s", PAPI_strerror(benchmark->end.papi_rc1));
			}
		}

		fprintf(file, "\n");
	}

	if (file != stdout) {
		fclose(file);
	}

leave:
	(*env)->ReleaseStringUTFChars(env, jfilename, filename);
}
