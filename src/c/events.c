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
#include <errno.h>
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

#ifdef HAS_TIMESPEC
static inline long long timespec_diff_as_ns(const struct timespec *a, const struct timespec *b) {
	long long sec_diff = b->tv_sec - a->tv_sec;
	long long nanosec_diff = b->tv_nsec - a->tv_nsec;
	return sec_diff * 1000 * 1000 * 1000 + nanosec_diff;
}
#endif

static long long timestamp_diff_ns(const timestamp_t *a, const timestamp_t *b) {
#ifdef HAS_TIMESPEC
	return timespec_diff_as_ns(a, b);
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

#ifdef HAS_GET_THREAD_TIMES
static long long get_filetime_diff_in_us(const FILETIME *a, const FILETIME *b) {
  long long result = ((LARGE_INTEGER*)b)->QuadPart - ((LARGE_INTEGER*)a)->QuadPart;
  return result / 10;
}
#endif

static long long getter_thread_time(const benchmark_run_t *bench, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
#ifdef HAS_TIMESPEC
	long long result = timespec_diff_as_ns(&bench->start.threadtime, &bench->end.threadtime);
	// fprintf(stderr, "getter_thread_time(%lld:%lld, %lld:%lld) = %lld\n", (long long) bench->start.threadtime.tv_sec, (long long) bench->start.threadtime.tv_nsec, (long long) bench->end.threadtime.tv_sec, (long long) bench->end.threadtime.tv_nsec, result);
	return result;
#elif defined(HAS_GET_THREAD_TIMES)
  long long kernel_us = get_filetime_diff_in_us(&bench->start.threadtime.kernel, &bench->end.threadtime.kernel);
  long long user_us = get_filetime_diff_in_us(&bench->start.threadtime.user, &bench->end.threadtime.user);
  return (user_us + kernel_us) * 1000;
#else
	return 0;
#endif
}

static long long getter_jvm_compilations(const benchmark_run_t *bench, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	return bench->end.compilations - bench->start.compilations;
}

#ifdef HAS_GETRUSAGE
static long long getter_context_switch_forced(const benchmark_run_t *bench, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	return bench->end.resource_usage.ru_nivcsw - bench->start.resource_usage.ru_nivcsw;
}

static inline long long timeval_diff_as_us(const struct timeval *a, const struct timeval *b) {
	// fprintf(stderr, "timeval_diff_as_us(%lld:%lld, %lld:%lld)\n", (long long) a->tv_sec, (long long) a->tv_usec, (long long) b->tv_sec, (long long) b->tv_usec);
	long long sec_diff = b->tv_sec - a->tv_sec;
	long long usec_diff = b->tv_usec - a->tv_usec;

	return sec_diff * 1000 * 1000 + usec_diff;
}

static long long getter_thread_time_rusage(const benchmark_run_t *bench, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	long long user_us = timeval_diff_as_us(&bench->start.resource_usage.ru_utime, &bench->end.resource_usage.ru_utime);
	long long system_us = timeval_diff_as_us(&bench->start.resource_usage.ru_stime, &bench->end.resource_usage.ru_stime);
	return (user_us + system_us) * (long long) 1000;
}

#endif

#ifdef HAS_PAPI
static long long getter_papi(const benchmark_run_t *bench, const ubench_event_info_t *info) {
	if (bench->start.papi_rc1 != PAPI_OK) {
		return bench->start.papi_rc1;
	} else if (bench->start.papi_rc2 != PAPI_OK) {
		return bench->start.papi_rc2;
	} else if (bench->end.papi_rc1 != PAPI_OK) {
		return bench->end.papi_rc1;
	}

	long long result = bench->end.papi_events[info->papi_index] - bench->start.papi_events[info->papi_index];
	if (result < 0) {
		// FIXME: can this happen?
		return result;
	} else {
		return result;
	}
}

static int resolve_papi_event(const char *name, ubench_event_info_t *info) {
	int papi_event_id = 0;
	int ok = PAPI_event_name_to_code((char *) name, &papi_event_id);
	if (ok != PAPI_OK) {
		return 0;
	}

	info->backend = UBENCH_EVENT_BACKEND_PAPI;
	info->id = papi_event_id;
	info->papi_component = 0;
	info->op_get = getter_papi;
	info->name = ubench_str_dup(name);
	return 1;
}

static int resolve_papi_component(const char *name_or_number) {
	int component_index = PAPI_get_component_index((char *)name_or_number);
	if (component_index >= 0) {
		return component_index;
	}

	char *end_of_number = NULL;
	component_index = (int) strtol(name_or_number, &end_of_number, 0);
	if (*end_of_number == 0) {
		if (PAPI_get_component_info(component_index) != NULL) {
			return component_index;
		}
	}

	return PAPI_ENOCMP;
}
#endif

int ubench_event_resolve(const char *event, ubench_event_info_t *info) {
	if (event == NULL) {
		return 0;
	}

	if (ubench_str_is_icase_equal(event, "SYS:wallclock-time")) {
		info->backend = UBENCH_EVENT_BACKEND_SYS_WALLCLOCK;
		info->op_get = getter_wall_clock_time;
		info->name = ubench_str_dup(event);
		return 1;
	} else if (ubench_str_is_icase_equal(event, "SYS:thread-time")) {
		info->backend = UBENCH_EVENT_BACKEND_SYS_THREADTIME;
		info->op_get = getter_thread_time;
		info->name = ubench_str_dup(event);
		return 1;
	} else if (ubench_str_is_icase_equal(event, "SYS:thread-time-rusage")) {
#ifdef HAS_GETRUSAGE
		info->backend = UBENCH_EVENT_BACKEND_RESOURCE_USAGE;
		info->op_get = getter_thread_time_rusage;
		info->name = ubench_str_dup(event);
		return 1;
#else
		return 0;
#endif
	} else if (ubench_str_is_icase_equal(event, "SYS:forced-context-switches")) {
#ifdef HAS_GETRUSAGE
		info->backend = UBENCH_EVENT_BACKEND_RESOURCE_USAGE;
		info->op_get = getter_context_switch_forced;
		info->name = ubench_str_dup(event);
		return 1;
#else
		return 0;
#endif
	} else if (ubench_str_is_icase_equal(event, "JVM:compilations")) {
		info->backend = UBENCH_EVENT_BACKEND_JVM_COMPILATIONS;
		info->op_get = getter_jvm_compilations;
		info->name = ubench_str_dup(event);
		return 1;
	} else if (ubench_str_starts_with_icase(event, "PAPI:")) {
#ifdef HAS_PAPI
		return resolve_papi_event(event + 5, info);
#else
		return 0;
#endif
	} else if (ubench_str_starts_with_icase(event, "PAPI/")) {
#ifdef HAS_PAPI
		// Parse PAPI component name.
		char *component_name = ubench_str_dup(event + 5);
		if (component_name == NULL) {
			return 0;
		}

		char *colon_position = strchr(component_name, ':');
		if (colon_position == NULL) {
			free(component_name);
			return 0;
		}
		*colon_position = 0;
		int component_index = resolve_papi_component(component_name);

		if (component_index < 0) {
			free(component_name);
			return 0;
		}

		colon_position++;

		int ok = resolve_papi_event(colon_position, info);
		free(component_name);

		if (!ok) {
			return 0;
		}

		info->papi_component = component_index;

		return 1;
#else
		return 0;
#endif
	}

	/*
	 * Legacy names.
	 */
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
	/*
	 * Let's try PAPI as fallback for any other event name.
	 */
	int papi_event_ok = resolve_papi_event(event, info);
	if (papi_event_ok) {
		return 1;
	}
#endif

	return 0;
}
