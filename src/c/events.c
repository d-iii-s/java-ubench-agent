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

typedef int (*resolve_event_func_t)(const char *, ubench_event_info_t *);
typedef int (*event_lister_func_t)(event_info_iterator_callback_t, void *);

typedef struct {
	const char *name;
	int obsolete;
	resolve_event_func_t resolver;
	event_lister_func_t lister;
	unsigned int backend;
	event_getter_raw_func_t getter_raw;
	event_getter_func_t getter;
} known_event_t;


#define TIMESPEC_TO_NANOS(val) ((val).tv_sec * 1000 * 1000 * 1000 + (val).tv_nsec)
#define TIMEVAL_TO_MICROS(val) ((val).tv_sec * 1000 * 1000 + (val).tv_usec)

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

static long long getter_wall_clock_time(const ubench_events_snapshot_t *start, const ubench_events_snapshot_t *end, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	return timestamp_diff_ns(&start->timestamp, &end->timestamp);
}

static long long getter_raw_wall_clock_time(const ubench_events_snapshot_t *value, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
#ifdef HAS_TIMESPEC
	return TIMESPEC_TO_NANOS(value->timestamp);
#elif defined(HAS_QUERY_PERFORMANCE_COUNTER)
	if (windows_timer_frequency.QuadPart == 0) {
		return -1;
	}
	return value->timestamp.QuadPart * 1000 * 1000 * 1000 / windows_timer_frequency.QuadPart;
#else
	return value->timestamp;
#endif
}

#ifdef HAS_GET_THREAD_TIMES
static long long get_filetime_diff_in_us(const FILETIME *a, const FILETIME *b) {
  long long result = ((LARGE_INTEGER*)b)->QuadPart - ((LARGE_INTEGER*)a)->QuadPart;
  return result / 10;
}

static long long get_filetime_in_us(const FILETIME *val) {
	return (((LARGE_INTEGER*)val)->QuadPart) / 10;
}
#endif

static long long getter_thread_time(const ubench_events_snapshot_t *start, const ubench_events_snapshot_t *end, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
#ifdef HAS_TIMESPEC
	long long result = timespec_diff_as_ns(&start->threadtime, &end->threadtime);
	// fprintf(stderr, "getter_thread_time(%lld:%lld, %lld:%lld) = %lld\n", (long long) bench->start.threadtime.tv_sec, (long long) bench->start.threadtime.tv_nsec, (long long) bench->end.threadtime.tv_sec, (long long) bench->end.threadtime.tv_nsec, result);
	return result;
#elif defined(HAS_GET_THREAD_TIMES)
  long long kernel_us = get_filetime_diff_in_us(&start->threadtime.kernel, &end->threadtime.kernel);
  long long user_us = get_filetime_diff_in_us(&start->threadtime.user, &end->threadtime.user);
  return (user_us + kernel_us) * 1000;
#else
	return 0;
#endif
}

static long long getter_raw_thread_time(const ubench_events_snapshot_t *value, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
#ifdef HAS_TIMESPEC
	return value->threadtime.tv_sec * 1000 * 1000 * 1000 + value->threadtime.tv_nsec;
#elif defined(HAS_GET_THREAD_TIMES)
	return (get_filetime_in_us(&value->threadtime.kernel) + get_filetime_in_us(&value->threadtime.user)) * 1000;
#else
	return 0;
#endif
}

static long long getter_jvm_compilations(const ubench_events_snapshot_t *start, const ubench_events_snapshot_t *end, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	return end->compilations - start->compilations;
}

static long long getter_raw_jvm_compilations(const ubench_events_snapshot_t *value, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	return value->compilations;
}


#ifdef HAS_GETRUSAGE
static long long getter_context_switch_forced(const ubench_events_snapshot_t *start, const ubench_events_snapshot_t *end, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	return end->resource_usage.ru_nivcsw - start->resource_usage.ru_nivcsw;
}

static long long getter_raw_context_switch_forced(const ubench_events_snapshot_t *value, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	return value->resource_usage.ru_nivcsw;
}

static inline long long timeval_diff_as_us(const struct timeval *a, const struct timeval *b) {
	// fprintf(stderr, "timeval_diff_as_us(%lld:%lld, %lld:%lld)\n", (long long) a->tv_sec, (long long) a->tv_usec, (long long) b->tv_sec, (long long) b->tv_usec);
	long long sec_diff = b->tv_sec - a->tv_sec;
	long long usec_diff = b->tv_usec - a->tv_usec;

	return sec_diff * 1000 * 1000 + usec_diff;
}

static long long getter_thread_time_rusage(const ubench_events_snapshot_t *start, const ubench_events_snapshot_t *end, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	long long user_us = timeval_diff_as_us(&start->resource_usage.ru_utime, &end->resource_usage.ru_utime);
	long long system_us = timeval_diff_as_us(&start->resource_usage.ru_stime, &end->resource_usage.ru_stime);
	return (user_us + system_us) * (long long) 1000;
}

static long long getter_raw_thread_time_rusage(const ubench_events_snapshot_t *value, const ubench_event_info_t *UNUSED_PARAMETER(info)) {
	long long user_us = TIMEVAL_TO_MICROS(value->resource_usage.ru_utime);
	long long system_us = TIMEVAL_TO_MICROS(value->resource_usage.ru_stime);
	return (user_us + system_us) * (long long) 1000;
}

#endif

#ifdef HAS_PAPI
static long long getter_papi(const ubench_events_snapshot_t *start, const ubench_events_snapshot_t *end, const ubench_event_info_t *info) {
	if (start->papi_rc1 != PAPI_OK) {
		return start->papi_rc1;
	} else if (start->papi_rc2 != PAPI_OK) {
		return start->papi_rc2;
	} else if (end->papi_rc1 != PAPI_OK) {
		return end->papi_rc1;
	}

	long long result = end->papi_events[info->papi_index] - start->papi_events[info->papi_index];
	if (result < 0) {
		// FIXME: can this happen?
		return result;
	} else {
		return result;
	}
}

static long long getter_raw_papi(const ubench_events_snapshot_t *value, const ubench_event_info_t *info) {
	if (value->papi_rc1 != PAPI_OK) {
		return value->papi_rc1;
	} else if (value->papi_rc2 != PAPI_OK) {
		return value->papi_rc2;
	}

	return value->papi_events[info->papi_index];
}

static int resolve_papi_event(const char *name, ubench_event_info_t *info) {
	int papi_event_id = 0;
	int ok = PAPI_event_name_to_code((char *) name, &papi_event_id);
	if (ok != PAPI_OK) {
		return 0;
	}
	PAPI_event_info_t event_info;
	ok = PAPI_get_event_info(papi_event_id, &event_info);
	if (ok != PAPI_OK) {
		return 0;
	}

	if (IS_PRESET(papi_event_id) && !event_info.count) {
		return 0;
	}

	info->id = papi_event_id;
	info->papi_component = event_info.component_index;
	return 1;
}

static int resolve_papi_event_with_prefix(const char *name, ubench_event_info_t *info) {
	return resolve_papi_event(name + 5, info);
}

static int list_papi_events(event_info_iterator_callback_t callback, void *arg) {
	char event_name_full[PAPI_MAX_STR_LEN + 7 ];
	strcpy(event_name_full, "PAPI:");
	char *event_name = event_name_full + strlen(event_name_full);

	int rc;
	int event_code;

	// First, retrieve available preset events
	event_code = 0 | PAPI_PRESET_MASK;

	for (rc = PAPI_enum_event(&event_code, PAPI_ENUM_FIRST);
			rc == PAPI_OK;
			rc = PAPI_enum_event(&event_code, PAPI_PRESET_ENUM_AVAIL)) {
		rc = PAPI_event_code_to_name(event_code, event_name);
		if (rc != PAPI_OK) {
			continue;
		}
		PAPI_event_info_t event_info;
		rc = PAPI_get_event_info(event_code, &event_info);
		if (rc != PAPI_OK) {
			continue;
		}
		if (IS_PRESET(event_code) && !event_info.count) {
			continue;
		}

		int terminate = callback(event_name_full, arg);
		if (terminate) {
			return 1;
		}
	}

	// Next, iterate over all components and print their events
	int component_count = PAPI_num_components();
	for (int component = 0; component < component_count; component++) {
		event_code = 0 | PAPI_NATIVE_MASK;
		for (rc = PAPI_enum_cmp_event(&event_code, PAPI_ENUM_FIRST, component);
				rc == PAPI_OK;
				rc = PAPI_enum_cmp_event(&event_code, PAPI_ENUM_EVENTS, component)) {
			rc = PAPI_event_code_to_name(event_code, event_name);
			if (rc != PAPI_OK) {
				continue;
			}
			int terminate = callback(event_name_full, arg);
			if (terminate) {
				return 1;
			}
		}
	}

	return 0;
}
#endif


static known_event_t known_events[] = {
	/* Legacy names first. */
	{
		.name = "JVM_COMPILATIONS",
		.obsolete = 1,
		.resolver = NULL,
		.lister = NULL,
		.backend = UBENCH_EVENT_BACKEND_JVM_COMPILATIONS,
		.getter_raw = getter_raw_jvm_compilations,
		.getter = getter_jvm_compilations
	},
	{
		.name = "SYS_WALLCLOCK",
		.obsolete = 1,
		.resolver = NULL,
		.lister = NULL,
		.backend = UBENCH_EVENT_BACKEND_SYS_WALLCLOCK,
		.getter_raw = getter_raw_wall_clock_time,
		.getter = getter_wall_clock_time
	},
#ifdef HAS_GETRUSAGE
	{
		.name = "forced-context-switch",
		.obsolete = 1,
		.resolver = NULL,
		.lister = NULL,
		.backend = UBENCH_EVENT_BACKEND_RESOURCE_USAGE,
		.getter_raw = getter_raw_context_switch_forced,
		.getter = getter_context_switch_forced
	},
#endif


	/* New naming. */
	{
		.name = "JVM:compilations",
		.obsolete = 0,
		.resolver = NULL,
		.lister = NULL,
		.backend = UBENCH_EVENT_BACKEND_JVM_COMPILATIONS,
		.getter_raw = getter_raw_jvm_compilations,
		.getter = getter_jvm_compilations
	},
#ifdef HAS_PAPI
	{
		.name = "PAPI:",
		.obsolete = 0,
		.resolver = resolve_papi_event_with_prefix,
		.lister = list_papi_events,
		.backend = UBENCH_EVENT_BACKEND_PAPI,
		.getter_raw = getter_raw_papi,
		.getter = getter_papi
	},
#endif
#ifdef HAS_GETRUSAGE
	{
		.name = "SYS:forced-context-switches",
		.obsolete = 0,
		.resolver = NULL,
		.lister = NULL,
		.backend = UBENCH_EVENT_BACKEND_RESOURCE_USAGE,
		.getter_raw = getter_raw_context_switch_forced,
		.getter = getter_context_switch_forced
	},
#endif
	{
		.name = "SYS:thread-time",
		.obsolete = 0,
		.resolver = NULL,
		.lister = NULL,
		.backend = UBENCH_EVENT_BACKEND_SYS_THREADTIME,
		.getter_raw = getter_raw_thread_time,
		.getter = getter_thread_time
	},
#ifdef HAS_GETRUSAGE
	{
		.name = "SYS:thread-time-rusage",
		.obsolete = 0,
		.resolver = NULL,
		.lister = NULL,
		.backend = UBENCH_EVENT_BACKEND_RESOURCE_USAGE,
		.getter_raw = getter_raw_thread_time_rusage,
		.getter = getter_thread_time_rusage
	},
#endif
	{
		.name = "SYS:wallclock-time",
		.obsolete = 0,
		.resolver = NULL,
		.lister = NULL,
		.backend = UBENCH_EVENT_BACKEND_SYS_WALLCLOCK,
		.getter_raw = getter_raw_wall_clock_time,
		.getter = getter_wall_clock_time
	},
#ifdef HAS_PAPI
	{
		.name = "",
		.obsolete = 1,
		.resolver = resolve_papi_event,
		.lister = NULL,
		.backend = UBENCH_EVENT_BACKEND_PAPI,
		.getter_raw = getter_raw_papi,
		.getter = getter_papi
	},
#endif
	{
		.name = NULL,
		.resolver = NULL,
		.lister = NULL,
		.backend = 0,
		.getter_raw = NULL,
		.getter = NULL
	}
};


int ubench_event_resolve(const char *event, ubench_event_info_t *info) {
	if (event == NULL) {
		return 0;
	}

	for (known_event_t *it = known_events; it->name != NULL; it++) {
		if (it->resolver == NULL) {
			if (!ubench_str_is_icase_equal(event, it->name)) {
				continue;
			}
		} else {
			if (!ubench_str_starts_with_icase(event, it->name)) {
				continue;
			}
			if (!it->resolver(event, info)) {
				continue;
			}
		}
		info->backend = it->backend;
		info->op_get_raw = it->getter_raw;
		info->op_get = it->getter;
		info->name = ubench_str_dup(event);
		return 1;
	}

	return 0;
}

void ubench_event_iterate(event_info_iterator_callback_t iterator_callback, void *arg) {
	if (iterator_callback == NULL) {
		return;
	}

	for (known_event_t *it = known_events; it->name != NULL; it++) {
		if (it->obsolete) {
			continue;
		}
		if (it->lister != NULL) {
			int terminate = it->lister(iterator_callback, arg);
			if (terminate) {
				return;
			}
		} else if (it->resolver == NULL) {
			int terminate = iterator_callback(it->name, arg);
			if (terminate) {
				return;
			}
		} else {
			// No listing but resolver set: skipping
		}
	}
}
