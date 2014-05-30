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
} metric_snapshot_t;

typedef struct {
	metric_snapshot_t start;
	metric_snapshot_t end;
} benchmark_run_t;

typedef void (*metric_dump_func_t)(FILE *, const benchmark_run_t *);
typedef struct {
	const char *name;
	metric_dump_func_t func;
} metric_dump_func_name_t;



static void dump_timestamp_diff(FILE *output, const benchmark_run_t *bench) {
	fprintf(output, "%15lld", timestamp_diff_ns(&bench->start.timestamp, &bench->end.timestamp));
}

static void dump_timestamp_start(FILE *output, const benchmark_run_t *bench) {
	fprintf(output, PRI_TIMESTAMP_FMT, PRI_TIMESTAMP(bench->start.timestamp));
}

static void dump_timestamp_stop(FILE *output, const benchmark_run_t *bench) {
	fprintf(output, PRI_TIMESTAMP_FMT, PRI_TIMESTAMP(bench->end.timestamp));
}

static void dump_voluntary_contextswitch_diff(FILE *output, const benchmark_run_t *bench) {
	fprintf(output, "%5ld",
#ifdef USE_GETRUSAGE
		bench->end.resource_usage.ru_nvcsw - bench->start.resource_usage.ru_nvcsw
#else
		NOTSUP_LONG
#endif
	);
}

static void dump_involuntary_contextswitch_diff(FILE *output, const benchmark_run_t *bench) {
	fprintf(output, "%5ld",
#ifdef USE_GETRUSAGE
		bench->end.resource_usage.ru_nivcsw - bench->start.resource_usage.ru_nivcsw
#else
		NOTSUP_LONG
#endif
	);
}

static void dump_compilation_diff(FILE *output, const benchmark_run_t *bench) {
	fprintf(output, "%10ld", bench->end.compilations - bench->start.compilations);
}

static metric_dump_func_name_t dump_functions[] = {
	{ .name = "timestamp-diff", .func = dump_timestamp_diff },
	{ .name = "timestamp-start", .func = dump_timestamp_start },
	{ .name = "timestamp-stop", .func = dump_timestamp_stop },
	{ .name = "voluntarycontextswitch-diff", .func = dump_voluntary_contextswitch_diff },
	{ .name = "involuntarycontextswitch-diff", .func = dump_involuntary_contextswitch_diff },
	{ .name = "compilation-diff", .func = dump_compilation_diff },
	// { .name = "", .func = dump_ },
	{ .name = NULL, .func = NULL }
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
	benchmark_runs[benchmark_runs_index].start.compilations = compilation_counter_get_compile_count();
	store_current_timestamp(&benchmark_runs[benchmark_runs_index].start.timestamp);
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_stop(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	store_current_timestamp(&benchmark_runs[benchmark_runs_index].end.timestamp);
	benchmark_runs[benchmark_runs_index].end.compilations = compilation_counter_get_compile_count();
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
	metric_dump_func_t *dump_funcs = malloc(sizeof(metric_dump_func_t) * format_count);
	size_t dump_funcs_count = 0;

	/* Parse the format. */
	char *format_copy = strdup(format);
	char *format_copy_or_null = format_copy;
	while (1) {
		char *token = strtok(format_copy_or_null, ",");
		format_copy_or_null = NULL;
		if (token == NULL) {
			break;
		}

		metric_dump_func_name_t *func_it = dump_functions;
		while (func_it->name != NULL) {
			if (strcmp(token, func_it->name) == 0) {
				dump_funcs[dump_funcs_count] = func_it->func;
				dump_funcs_count++;
				break;
			}
			func_it++;
		}
	}

	free(format_copy);

	/* Print the results. */
	for (size_t bi = 0; bi < benchmark_runs_index; bi++) {
		for (size_t fi = 0; fi < dump_funcs_count; fi++) {
			dump_funcs[fi](file, &benchmark_runs[bi]);
		}
		fprintf(file, "\n");
	}

	if (file != stdout) {
		fclose(file);
	}

leave:
	free(dump_funcs);
	(*env)->ReleaseStringUTFChars(env, jfilename, filename);
	(*env)->ReleaseStringUTFChars(env, jformat, format);
}
