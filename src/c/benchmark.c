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
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <string.h>
#include <sys/resource.h>

#ifdef HAS_PAPI
#include <papi.h>
#endif

typedef struct timespec timestamp_t;
#define PRI_TIMESTAMP_FMT "%6ld.%09ld"
#define PRI_TIMESTAMP(x) (x).tv_sec, (x).tv_nsec

#define UBENCH_MAX_PAPI_EVENTS 20

#define UBENCH_EVENT_BACKEND_LINUX 1
#define UBENCH_EVENT_BACKEND_RESOURCE_USAGE 2
#define UBENCH_EVENT_BACKEND_PAPI 4

typedef struct {
	timestamp_t timestamp;
	struct rusage resource_usage;
	long compilations;
	int garbage_collections;
#ifdef HAS_PAPI
	long long papi_events[UBENCH_MAX_PAPI_EVENTS];
	int papi_rc1;
	int papi_rc2;
#endif
} ubench_events_snapshot_t;

typedef struct {
	ubench_events_snapshot_t start;
	ubench_events_snapshot_t end;
} benchmark_run_t;

typedef struct ubench_event_info ubench_event_info_t;
typedef long long (*event_getter_func_t)(const benchmark_run_t *, const ubench_event_info_t *);

struct ubench_event_info {
	unsigned int backend;
	int id;
	size_t papi_index;
	event_getter_func_t op_get;
};

typedef struct {
	unsigned int used_backends;

	ubench_event_info_t *used_events;
	size_t used_events_count;

#ifdef HAS_PAPI
	int used_papi_events[UBENCH_MAX_PAPI_EVENTS];
	size_t used_papi_events_count;
#endif

	benchmark_run_t *data;
	size_t data_size;
	size_t data_index;
} benchmark_configuration_t;

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

#ifdef HAS_PAPI
static long long getter_papi(const benchmark_run_t *bench, const ubench_event_info_t *info) {
	if (bench->start.papi_rc1 != PAPI_OK) {
		return -1;
	} else if (bench->start.papi_rc2 != PAPI_OK) {
		return -1;
	} else if (bench->end.papi_rc1 != PAPI_OK) {
		return -1;
	}

	return bench->end.papi_events[info->papi_index] - bench->start.papi_events[info->papi_index];
}
#endif

jint ubench_benchmark_init(void) {
#ifdef HAS_PAPI
	// TODO: check for errors
	PAPI_library_init(PAPI_VER_CURRENT);
#endif

	current_benchmark.used_backends = 0;
	current_benchmark.used_events = NULL;
	current_benchmark.used_events_count = 0;
#ifdef HAS_PAPI
	current_benchmark.used_papi_events_count = 0;
#endif

	current_benchmark.data = NULL;
	current_benchmark.data_index = 0;
	current_benchmark.data_size = 0;

	return JNI_OK;
}

static int resolve_event(const char *event, ubench_event_info_t *info) {
	if (event == NULL) {
		return 0;
	}

	if (strcmp(event, "clock-monotonic") == 0) {
		info->backend = UBENCH_EVENT_BACKEND_LINUX;
		info->op_get = getter_wall_clock_time;
		return 1;
	} else if (strcmp(event, "forced-context-switch") == 0) {
		info->backend = UBENCH_EVENT_BACKEND_RESOURCE_USAGE;
		info->op_get = getter_context_switch_forced;
		return 1;
	}

#ifdef HAS_PAPI
	/* Let's try PAPI */
	int papi_event_id = 0;
	int ok = PAPI_event_name_to_code((char *) event, &papi_event_id);
	if (ok == PAPI_OK) {
		info->backend = UBENCH_EVENT_BACKEND_PAPI;
		info->id = papi_event_id;
		info->op_get = getter_papi;
		return 1;
	}
#endif

	return 0;
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_init(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass),
		jint jmeasurements, jobjectArray jeventNames) {
	free(current_benchmark.data);
	free(current_benchmark.used_events);
	current_benchmark.data = NULL;
	current_benchmark.data_index = 0;
	current_benchmark.data_size = 0;

	current_benchmark.used_backends = 0;
	current_benchmark.used_events_count = 0;
#ifdef HAS_PAPI
	current_benchmark.used_papi_events_count = 0;
#endif

	size_t events_count = (*env)->GetArrayLength(env, jeventNames);
	if (events_count == 0) {
		return;
	}

	if (jmeasurements <= 0) {
		return;
	}

	current_benchmark.data = malloc(sizeof(benchmark_run_t) * jmeasurements);
	current_benchmark.data_size = jmeasurements;

	current_benchmark.used_events = malloc(sizeof(ubench_event_info_t) * events_count);

	for (size_t i = 0; i < events_count; i++) {
		jstring jevent_name = (jstring) (*env)->GetObjectArrayElement(env, jeventNames, i);
		const char *event_name = (*env)->GetStringUTFChars(env, jevent_name, 0);

		ubench_event_info_t *event_info = &current_benchmark.used_events[current_benchmark.used_events_count];

		int event_ok = resolve_event(event_name, event_info);
		if (!event_ok) {
			fprintf(stderr, "Unrecognized event %s\n", event_name);
			goto event_loop_end;
		}

		current_benchmark.used_events_count++;

		current_benchmark.used_backends |= event_info->backend;

#ifdef HAS_PAPI
		if (event_info->backend == UBENCH_EVENT_BACKEND_PAPI) {
			/* Check that the id is not already there. */
			int already_registered = 0;
			for (size_t j = 0; j < current_benchmark.used_papi_events_count; j++) {
				if (current_benchmark.used_papi_events[j] == event_info->id) {
					already_registered = 1;
					event_info->papi_index = j;
					break;
				}
			}
			if (!already_registered) {
				if (current_benchmark.used_papi_events_count < UBENCH_MAX_PAPI_EVENTS) {
					event_info->papi_index = current_benchmark.used_papi_events_count;
					current_benchmark.used_papi_events[current_benchmark.used_papi_events_count] = event_info->id;
					current_benchmark.used_papi_events_count++;
				}
				// FIXME: inform user that there are way to many PAPI events
			}
		}
#endif

event_loop_end:
		(*env)->ReleaseStringUTFChars(env, jevent_name, event_name);
	}

#if 0
	for (size_t i = 0; i < current_benchmark.used_events_count; i++) {
		int event_id = current_benchmark.used_events[i];
		fprintf(stderr, "Event #%2zu: %s\n", i, all_known_events_info[event_id].id);
	}
#endif
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

	if ((current_benchmark.used_backends & UBENCH_EVENT_BACKEND_RESOURCE_USAGE) > 0) {
		getrusage(RUSAGE_SELF, &(snapshot->resource_usage));
	}

	snapshot->compilations = ubench_atomic_get(&counter_compilation_total);
	snapshot->garbage_collections = ubench_atomic_get(&counter_gc_total);

#ifdef HAS_PAPI
	if ((current_benchmark.used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		// TODO: check for errors
		snapshot->papi_rc1 = PAPI_start_counters(current_benchmark.used_papi_events, current_benchmark.used_papi_events_count);
		snapshot->papi_rc2 = PAPI_read_counters(snapshot->papi_events, current_benchmark.used_papi_events_count);
	}
#endif

	store_current_timestamp(&(snapshot->timestamp));
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_stop(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	ubench_events_snapshot_t *snapshot = &current_benchmark.data[current_benchmark.data_index].end;

	store_current_timestamp(&(snapshot->timestamp));

#ifdef HAS_PAPI
	// TODO: check for errors
	if ((current_benchmark.used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		snapshot->papi_rc1 = PAPI_stop_counters(snapshot->papi_events, current_benchmark.used_papi_events_count);
	}
#endif

	snapshot->compilations = ubench_atomic_get(&counter_compilation_total);
	snapshot->garbage_collections = ubench_atomic_get(&counter_gc_total);

	if ((current_benchmark.used_backends & UBENCH_EVENT_BACKEND_RESOURCE_USAGE) > 0) {
		getrusage(RUSAGE_SELF, &(snapshot->resource_usage));
	}

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
			ubench_event_info_t *event = &current_benchmark.used_events[ei];
			long long value = event->op_get(benchmark, event);
			fprintf(file, "%12lld", value);
		}

#ifdef HAS_PAPI
		if (current_benchmark.used_backends & UBENCH_EVENT_BACKEND_PAPI) {
			if (benchmark->start.papi_rc1 != PAPI_OK) {
				fprintf(file, " start_counters=%s", PAPI_strerror(benchmark->start.papi_rc1));
			} else if (benchmark->start.papi_rc2 != PAPI_OK) {
				fprintf(file, " read_counters=%s", PAPI_strerror(benchmark->start.papi_rc2));
			} else if (benchmark->end.papi_rc1 != PAPI_OK) {
				fprintf(file, " stop_counters=%s", PAPI_strerror(benchmark->end.papi_rc1));
			}
		}
#endif

		fprintf(file, "\n");
	}

	if (file != stdout) {
		fclose(file);
	}

leave:
	(*env)->ReleaseStringUTFChars(env, jfilename, filename);
}
