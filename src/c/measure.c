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

#define _BSD_SOURCE // For glibc 2.18 to include caddr_t
#define _DEFAULT_SOURCE // For glibc 2.20 to include caddr_t
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "ubench.h"

#pragma warning(push, 0)
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <string.h>
#pragma warning(pop)

#ifdef HAS_PAPI
/*
 * Include <sys/types.h> to have caddr_t.
 * Not needed with GCC and -std=c99.
 */
#include <sys/types.h>
#include <papi.h>
#endif

#ifdef HAS_GETRUSAGE
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef HAS_QUERY_PERFORMANCE_COUNTER
#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)
#endif

static void store_wallclock(timestamp_t *ts) {
#ifdef HAS_TIMESPEC
	clock_gettime(CLOCK_MONOTONIC, ts);
#elif defined(HAS_QUERY_PERFORMANCE_COUNTER)
	QueryPerformanceCounter(ts);
#else
	*ts = -1;
#endif
}

static void store_threadtime(threadtime_t *ts) {
#ifdef HAS_TIMESPEC
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, ts);
#elif defined(HAS_GET_THREAD_TIMES)
  FILETIME t_creation, t_exit, t_kernel, t_user;
  HANDLE thr = GetCurrentThread();
  // For reasons yet unknown, directly passing &(ts->kernel), &(ts->user))
  // causes segfault. Thus, we use a temporary variable.
  GetThreadTimes(thr, &t_creation, &t_exit, &t_kernel, &t_user);
  ts->kernel = t_kernel;
  ts->user = t_user;
#else
	*ts = -1;
#endif
}

static inline void do_snapshot(const benchmark_configuration_t *config,
		ubench_events_snapshot_t *snapshot) {
#ifdef HAS_GETRUSAGE
	if ((config->used_backends & UBENCH_EVENT_BACKEND_RESOURCE_USAGE) > 0) {
		getrusage(RUSAGE_THREAD, &(snapshot->resource_usage));
	}
#endif

	if ((config->used_backends & UBENCH_EVENT_BACKEND_JVM_COMPILATIONS) > 0) {
		snapshot->compilations = ubench_atomic_int_get(&counter_compilation_total);
	}

	snapshot->garbage_collections = ubench_atomic_int_get(&counter_gc_total);

	if ((config->used_backends & UBENCH_EVENT_BACKEND_SYS_THREADTIME) > 0) {
		store_threadtime(&(snapshot->threadtime));
	}

#ifdef HAS_PAPI
	if ((config->used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		snapshot->papi_rc1 = PAPI_read(config->papi_eventset, snapshot->papi_events);
		DEBUG_PRINTF("PAPI_read(%d) = %d", config->papi_eventset, snapshot->papi_rc2);
	}
#endif

	if ((config->used_backends & UBENCH_EVENT_BACKEND_SYS_WALLCLOCK) > 0) {
		store_wallclock(&(snapshot->timestamp));
	}
}

void ubench_measure_start(const benchmark_configuration_t *config,
		ubench_events_snapshot_t *snapshot) {
#ifdef HAS_PAPI
	if ((config->used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		// TODO: check for errors
		snapshot->papi_rc2 = PAPI_start(config->papi_eventset);
		DEBUG_PRINTF("PAPI_start(%d) = %d", config->papi_eventset, snapshot->papi_rc1);
	}
#endif

	snapshot->type = UBENCH_SNAPSHOT_TYPE_START;
	do_snapshot(config, snapshot);
}

void ubench_measure_sample(const benchmark_configuration_t *config,
		ubench_events_snapshot_t *snapshot, int user_id) {
	snapshot->type = user_id;
	do_snapshot(config, snapshot);
}

void ubench_measure_stop(const benchmark_configuration_t *config,
		ubench_events_snapshot_t *snapshot) {
	if ((config->used_backends & UBENCH_EVENT_BACKEND_SYS_WALLCLOCK) > 0) {
		store_wallclock(&(snapshot->timestamp));
	}

#ifdef HAS_PAPI
	// TODO: check for errors
	if ((config->used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		snapshot->papi_rc1 = PAPI_stop(config->papi_eventset, snapshot->papi_events);
		DEBUG_PRINTF("PAPI_stop(%d) = %d", config->papi_eventset, snapshot->papi_rc1);
	}
#endif

	if ((config->used_backends & UBENCH_EVENT_BACKEND_SYS_THREADTIME) > 0) {
		store_threadtime(&(snapshot->threadtime));
	}

	if ((config->used_backends & UBENCH_EVENT_BACKEND_JVM_COMPILATIONS) > 0) {
		snapshot->compilations = ubench_atomic_int_get(&counter_compilation_total);
	}

	snapshot->garbage_collections = ubench_atomic_int_get(&counter_gc_total);

#ifdef HAS_GETRUSAGE
	if ((config->used_backends & UBENCH_EVENT_BACKEND_RESOURCE_USAGE) > 0) {
		getrusage(RUSAGE_THREAD, &(snapshot->resource_usage));
	}
#endif

	snapshot->type = UBENCH_SNAPSHOT_TYPE_END;
}
