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

#ifdef HAS_QUERY_PERFORMANCE_COUNTER
#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)
#endif

#ifdef HAS_PAPI
/*
 * Include <sys/types.h> to have caddr_t.
 * Not needed with GCC and -std=c99.
 */
#include <sys/types.h>
#include <papi.h>
#endif

#ifdef HAS_QUERY_PERFORMANCE_COUNTER
static LARGE_INTEGER windows_timer_frequency;
#endif

int ubench_event_init(void) {
#ifdef HAS_QUERY_PERFORMANCE_COUNTER
	 QueryPerformanceFrequency(&windows_timer_frequency);
#endif
	 return JNI_OK;
}

static long long timestamp_diff_ns(const timestamp_t *a, const timestamp_t *b) {
#ifdef HAS_TIMESPEC
	long long sec_diff = b->tv_sec - a->tv_sec;
	long long nanosec_diff = b->tv_nsec - a->tv_nsec;
	return sec_diff * 1000 * 1000 * 1000 + nanosec_diff;
#elif defined(HAS_QUERY_PERFORMANCE_COUNTER)
	if (windows_timer_frequency.QuadPart == 0) {
		return -1;
	}
	return (b->QuadPart - a->QuadPart) * 1000 * 1000 * 1000 / windows_timer_frequency.QuadPart;
#else
	return *b - *a;
#endif
}

static long long getter_wall_clock_time(const benchmark_run_t *bench, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	return timestamp_diff_ns(&bench->start.timestamp, &bench->end.timestamp);
}

static long long getter_jvm_compilations(const benchmark_run_t *bench, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	return bench->end.compilations - bench->start.compilations;
}

#ifdef HAS_GETRUSAGE
static long long getter_context_switch_forced(const benchmark_run_t *bench, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	return bench->end.resource_usage.ru_nivcsw - bench->start.resource_usage.ru_nivcsw;
}
#endif

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

int ubench_event_resolve(const char *event, ubench_event_info_t *info) {
	if (event == NULL) {
		return 0;
	}

	if (strcmp(event, "SYS_WALLCLOCK") == 0) {
		info->backend = UBENCH_EVENT_BACKEND_SYS_WALLCLOCK;
		info->op_get = getter_wall_clock_time;
		info->name = ubench_str_dup(event);
		return 1;
	}

	if (strcmp(event, "JVM_COMPILATIONS") == 0) {
		info->backend = UBENCH_EVENT_BACKEND_JVM_COMPILATIONS;
		info->op_get = getter_jvm_compilations;
		info->name = ubench_str_dup(event);
		return 1;
	}

#ifdef HAS_GETRUSAGE
	if (strcmp(event, "forced-context-switch") == 0) {
		info->backend = UBENCH_EVENT_BACKEND_RESOURCE_USAGE;
		info->op_get = getter_context_switch_forced;
		info->name = ubench_str_dup(event);
		return 1;
	}
#endif

#ifdef HAS_PAPI
	/* Let's try PAPI */
	int papi_event_id = 0;
	int ok = PAPI_event_name_to_code((char *) event, &papi_event_id);
	if (ok == PAPI_OK) {
		info->backend = UBENCH_EVENT_BACKEND_PAPI;
		info->id = papi_event_id;
		info->op_get = getter_papi;
		info->name = ubench_str_dup(event);
		return 1;
	}
#endif

	return 0;
}
