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

void ubench_measure_start(const benchmark_configuration_t const *config,
		ubench_events_snapshot_t *snapshot) {
#ifdef HAS_GETRUSAGE
	if ((config->used_backends & UBENCH_EVENT_BACKEND_RESOURCE_USAGE) > 0) {
		getrusage(RUSAGE_SELF, &(snapshot->resource_usage));
	}
#endif

	if ((config->used_backends & UBENCH_EVENT_BACKEND_JVM_COMPILATIONS) > 0) {
		snapshot->compilations = ubench_atomic_get(&counter_compilation_total);
	}

	snapshot->garbage_collections = ubench_atomic_get(&counter_gc_total);

#ifdef HAS_PAPI
	if ((config->used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		// TODO: check for errors
		snapshot->papi_rc1 = PAPI_start(config->papi_eventset);
		// Reset the counters but ignore the values!
		// Consider situation when PAPI_start_counters() by itself caused
		// the event but that would be the only place. We would then record
		// the count here and 0 (zero) in stop(). Thus the diff would be
		// negative.
		(void) PAPI_reset(config->papi_eventset);
	}
#endif

	if ((config->used_backends & UBENCH_EVENT_BACKEND_SYS_WALLCLOCK) > 0) {
		store_wallclock(&(snapshot->timestamp));
	}
}

void ubench_measure_stop(const benchmark_configuration_t const *config,
		ubench_events_snapshot_t *snapshot) {
	if ((config->used_backends & UBENCH_EVENT_BACKEND_SYS_WALLCLOCK) > 0) {
		store_wallclock(&(snapshot->timestamp));
	}

#ifdef HAS_PAPI
	// TODO: check for errors
	if ((config->used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		snapshot->papi_rc1 = PAPI_stop(config->papi_eventset, snapshot->papi_events);
	}
#endif

	if ((config->used_backends & UBENCH_EVENT_BACKEND_JVM_COMPILATIONS) > 0) {
		snapshot->compilations = ubench_atomic_get(&counter_compilation_total);
	}

	snapshot->garbage_collections = ubench_atomic_get(&counter_gc_total);

#ifdef HAS_GETRUSAGE
	if ((config->used_backends & UBENCH_EVENT_BACKEND_RESOURCE_USAGE) > 0) {
		getrusage(RUSAGE_SELF, &(snapshot->resource_usage));
	}
#endif
}
