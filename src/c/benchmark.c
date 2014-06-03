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

// TODO: replace later with some real check
#define USE_GETRUSAGE

#define NOTSUP_LONG ((long)-1)


#include "cz_cuni_mff_d3s_perf_CompilationCounter.h"
#include "microbench.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef USE_GETRUSAGE
#include <sys/resource.h>
#endif

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
#ifdef USE_GETRUSAGE
	struct rusage resource_usage;
#endif
	long compilations;
	int garbage_collections;
} metric_snapshot_t;

typedef struct {
	metric_snapshot_t start;
	metric_snapshot_t end;
} benchmark_run_t;

typedef long long (*metric_value_func_t)(const benchmark_run_t *);
typedef struct {
	const char *name;
	int width;
	metric_value_func_t get;
} metric_dump_func_name_t;

#ifdef USE_GETRUSAGE
#define GET_RUSAGE_DIFF(benchmark_run, field) \
	benchmark_run->end.resource_usage.field \
	- benchmark_run->start.resource_usage.field
#else
#define GET_RUSAGE_DIFF(benchmark_run, field) (long long) -1
#endif

static long long get_timestamp_diff(const benchmark_run_t *bench) {
	return timestamp_diff_ns(&bench->start.timestamp, &bench->end.timestamp);
}

static long long get_voluntary_contextswitch_diff(const benchmark_run_t *bench) {
	return GET_RUSAGE_DIFF(bench, ru_nvcsw);
}

static long long get_involuntary_contextswitch_diff(const benchmark_run_t *bench) {
	return GET_RUSAGE_DIFF(bench, ru_nivcsw);
}

static long long get_pagereclaim_diff(const benchmark_run_t *bench) {
	return GET_RUSAGE_DIFF(bench, ru_minflt);
}

static long long get_pagefault_diff(const benchmark_run_t *bench) {
	return GET_RUSAGE_DIFF(bench, ru_majflt);
}

static long long get_compilation_diff(const benchmark_run_t *bench) {
	return bench->end.compilations - bench->start.compilations;
}

static long long get_gc_diff(const benchmark_run_t *bench) {
	return bench->end.garbage_collections - bench->start.garbage_collections;
}

static metric_dump_func_name_t dump_functions[] = {
	{ .name = "timestamp-diff", .width = 10, .get = get_timestamp_diff },
	{ .name = "voluntarycontextswitch-diff", .width = 3, .get = get_voluntary_contextswitch_diff },
	{ .name = "involuntarycontextswitch-diff", .width = 3, .get = get_involuntary_contextswitch_diff },
	{ .name = "pagereclaim-diff", .width = 5, .get = get_pagereclaim_diff },
	{ .name = "pagefault-diff", .width = 3, .get = get_pagefault_diff },
	{ .name = "compilation-diff", .width = 3, .get = get_compilation_diff },
	{ .name = "gc-diff", .width = 3, .get = get_gc_diff },
	// { .name = "", .width = 0, .get = get_ },
	{ .name = NULL, .width = 0, .get = NULL }
};


static benchmark_run_t *benchmark_runs = NULL;
static size_t benchmark_runs_size = 0;
static size_t benchmark_runs_index = 0;

static inline void store_current_timestamp(timestamp_t *ts) {
	clock_gettime(CLOCK_MONOTONIC, ts);
}


void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_init(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass),
		jint jmeasurements) {
	size_t measurement_count = jmeasurements;
	if (benchmark_runs != NULL) {
		free(benchmark_runs);
	}

	benchmark_runs = malloc(sizeof(benchmark_run_t) * measurement_count);
	benchmark_runs_size = measurement_count;
	benchmark_runs_index = 0;
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_start(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	if (benchmark_runs_index >= benchmark_runs_size) {
		benchmark_runs_index = benchmark_runs_size - 1;
	}

#ifdef USE_GETRUSAGE
	getrusage(RUSAGE_SELF, &benchmark_runs[benchmark_runs_index].start.resource_usage);
#endif
	benchmark_runs[benchmark_runs_index].start.compilations = ubench_atomic_get(&counter_compilation_total);
	benchmark_runs[benchmark_runs_index].start.garbage_collections = ubench_atomic_get(&counter_gc_total);
	store_current_timestamp(&benchmark_runs[benchmark_runs_index].start.timestamp);
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_stop(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	store_current_timestamp(&benchmark_runs[benchmark_runs_index].end.timestamp);
	benchmark_runs[benchmark_runs_index].end.garbage_collections = ubench_atomic_get(&counter_gc_total);
	benchmark_runs[benchmark_runs_index].end.compilations = ubench_atomic_get(&counter_compilation_total);
#ifdef USE_GETRUSAGE
	getrusage(RUSAGE_SELF, &benchmark_runs[benchmark_runs_index].end.resource_usage);
#endif

	benchmark_runs_index++;
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_dumpFormatted(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass),
		jstring jfilename, jstring jformat) {
	const char *filename = (*env)->GetStringUTFChars(env, jfilename, 0);
	const char *format = (*env)->GetStringUTFChars(env, jformat, 0);

	FILE *file = NULL;
	if (strcmp(filename, "-") == 0) {
		file = stdout;
	} else {
		file = fopen(filename, "wt");
	}

	if (file == NULL) {
		goto leave;
	}

	/* Count length of the format. */
	int format_count = 0;
	{
		const char *pos = format;
		while (1) {
			format_count++;
			pos = strchr(pos, ',');
			if (pos == NULL) {
				break;
			}
			pos++;
		}
	}

	/* Prepare callback functions. */
	int *metrics_indices = malloc(sizeof(int) * format_count);
	size_t metrics_indices_count = 0;

	/* Parse the format. */
	char *format_copy = strdup(format);
	char *format_copy_or_null = format_copy;
	while (1) {
		char *token = strtok(format_copy_or_null, ",");
		format_copy_or_null = NULL;
		if (token == NULL) {
			break;
		}

		int index = 0;
		metric_dump_func_name_t *func_it = dump_functions;
		while (func_it->name != NULL) {
			if (strcmp(token, func_it->name) == 0) {
				metrics_indices[metrics_indices_count] = index;
				metrics_indices_count++;
				break;
			}
			index++;
			func_it++;
		}
	}

	free(format_copy);

	/* Print the results. */
	for (size_t bi = 0; bi < benchmark_runs_index; bi++) {
		for (size_t fi = 0; fi < metrics_indices_count; fi++) {
			metric_dump_func_name_t *dumper = &dump_functions[ metrics_indices[fi] ];
			long long value = dumper->get(&benchmark_runs[bi]);
			fprintf(file, "%*lld", dumper->width, value);
		}
		fprintf(file, "\n");
	}

	if (file != stdout) {
		fclose(file);
	}

leave:
	free(metrics_indices);
	(*env)->ReleaseStringUTFChars(env, jfilename, filename);
	(*env)->ReleaseStringUTFChars(env, jformat, format);
}
