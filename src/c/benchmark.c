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

#define  _POSIX_C_SOURCE 199309L

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

typedef struct {
	timestamp_t timestamp;
#ifdef USE_GETRUSAGE
	struct rusage resource_usage;
#endif
} metric_snapshot_t;

typedef struct {
	metric_snapshot_t start;
	metric_snapshot_t end;
} benchmark_run_t;

static benchmark_run_t *benchmark_runs = NULL;
static size_t benchmark_runs_size = 0;
static size_t benchmark_runs_index = 0;

static inline void store_current_timestamp(timestamp_t *ts) {
	clock_gettime(CLOCK_MONOTONIC, ts);
}

static inline long long timestamp_diff_ns(const timestamp_t *a, const timestamp_t *b) {
	long long sec_diff = b->tv_sec - a->tv_sec;
	long long nanosec_diff = b->tv_nsec - a->tv_nsec;
	return sec_diff * 1000 * 1000 * 1000 + nanosec_diff;
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
	store_current_timestamp(&benchmark_runs[benchmark_runs_index].start.timestamp);
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_stop(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	store_current_timestamp(&benchmark_runs[benchmark_runs_index].end.timestamp);
#ifdef USE_GETRUSAGE
	getrusage(RUSAGE_SELF, &benchmark_runs[benchmark_runs_index].end.resource_usage);
#endif

	benchmark_runs_index++;
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_dump(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass), jstring jfilename) {
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

	for (size_t i = 0; i < benchmark_runs_index; i++) {
		benchmark_run_t *this_run = &benchmark_runs[i];
		timestamp_t *start = &this_run->start.timestamp;
		timestamp_t *end = &this_run->end.timestamp;
		long long diff_ns = timestamp_diff_ns(start, end);

		fprintf(file, PRI_TIMESTAMP_FMT "  " PRI_TIMESTAMP_FMT \
			"  %15lld %10ld %10ld  %10ld %10ld\n",
			PRI_TIMESTAMP(*start), PRI_TIMESTAMP(*end),
			diff_ns,
#ifdef USE_GETRUSAGE
			this_run->start.resource_usage.ru_nvcsw,
			this_run->end.resource_usage.ru_nvcsw,
			this_run->start.resource_usage.ru_nivcsw,
			this_run->end.resource_usage.ru_nivcsw
#else
			NOTSUP_LONG, NOTSUP_LONG, NOTSUP_LONG, NOTSUP_LONG
#endif
		);
	}

	fclose(file);

leave:
	(*env)->ReleaseStringUTFChars(env, jfilename, filename);
}

