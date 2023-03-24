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

#ifndef UBENCH_H_GUARD
#define UBENCH_H_GUARD

#include "compiler.h"
#include "myatomic.h"
#include "mylock.h"

#pragma warning(push, 0)
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <jni.h>
#include <jvmti.h>
#pragma warning(pop)

#ifdef UBENCH_DEBUG
#define DEBUG_PRINTF(fmt, ...) printf("[ubench-agent]: " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) (void) 0
#endif

#ifdef HAS_GETRUSAGE
#include <sys/resource.h>
#endif

#ifdef HAS_TIMESPEC
typedef struct timespec timestamp_t;
#elif defined(HAS_QUERY_PERFORMANCE_COUNTER)
#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)
typedef LARGE_INTEGER timestamp_t;
#else
typedef int timestamp_t;
#endif

#ifdef HAS_TIMESPEC
typedef struct timespec threadtime_t;
#elif defined(HAS_GET_THREAD_TIMES)
#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)

typedef struct {
	FILETIME kernel;
	FILETIME user;
} threadtime_t;
#else
typedef int threadtime_t;
#endif

#define UBENCH_MAX_PAPI_EVENTS 20


/*
 * A number type that represents a Java thread identifier. This is just an
 * alias to the JNI 'jlong' type, which always represents a 64-bit integer.
 */
typedef jlong java_tid_t;

#define PRId_JAVA_TID PRId64

/*
 * A number type that represents a native thread identifier. The type must
 * be wide enough to contain various indentifier types, such as 'pid_t'
 * (32-bit signed), 'pthread_t' (unsigned long) or 'DWORD' (32-bit unsigned).
 */
typedef int_fast64_t native_tid_t;

#define PRId_NATIVE_TID PRIdFAST64

#define UBENCH_THREAD_ID_INVALID ((native_tid_t) -1)


/*
 * Backend bit masks.
 *
 * Backend refers to a function that collects the counters (be it number
 * of JIT compilations or number of page faults or ...).
 */
#define UBENCH_EVENT_BACKEND_LINUX 1
#define UBENCH_EVENT_BACKEND_RESOURCE_USAGE 2
#define UBENCH_EVENT_BACKEND_PAPI 4
#define UBENCH_EVENT_BACKEND_SYS_WALLCLOCK 8
#define UBENCH_EVENT_BACKEND_JVM_COMPILATIONS 16
#define UBENCH_EVENT_BACKEND_SYS_THREADTIME 32

#define UBENCH_SNAPSHOT_TYPE_START (-1)
#define UBENCH_SNAPSHOT_TYPE_END (-2)

typedef struct {
	timestamp_t timestamp;
	threadtime_t threadtime;
#ifdef HAS_GETRUSAGE
	struct rusage resource_usage;
#endif
	long compilations;
	int garbage_collections;
#ifdef HAS_PAPI
	long long papi_events[UBENCH_MAX_PAPI_EVENTS];
	int papi_rc1;
	int papi_rc2;
#endif
	int type;
} ubench_events_snapshot_t;

typedef struct ubench_event_info ubench_event_info_t;
typedef long long (*event_getter_raw_func_t)(const ubench_events_snapshot_t*, const ubench_event_info_t*);
typedef long long (*event_getter_func_t)(const ubench_events_snapshot_t*, const ubench_events_snapshot_t*, const ubench_event_info_t*);
typedef int (*event_info_iterator_callback_t)(const char*, void*);

struct ubench_event_info {
	unsigned int backend;
	int id;
	int papi_component;
	size_t papi_index;
	event_getter_raw_func_t op_get_raw;
	event_getter_func_t op_get;
	char* name;
};

typedef struct {
	unsigned int used_backends;

	ubench_event_info_t* used_events;
	size_t used_events_count;

#ifdef HAS_PAPI
	int used_papi_events[UBENCH_MAX_PAPI_EVENTS];
	size_t used_papi_events_count;
	int papi_eventset;
	int papi_component;
#endif

	ubench_events_snapshot_t* data;
	size_t data_size;
	size_t data_index;
} benchmark_configuration_t;

extern void ubench_jvm_callback_on_thread_start(jvmtiEnv*, JNIEnv*, jthread);
extern void ubench_jvm_callback_on_thread_end(jvmtiEnv*, JNIEnv*, jthread);

extern jint ubench_counters_init(jvmtiEnv*);
extern void ubench_register_this_thread(jthread, JNIEnv*);
extern void ubench_unregister_this_thread(jthread, JNIEnv*);
extern int ubench_register_thread_id_mapping(java_tid_t, native_tid_t);
extern int ubench_unregister_thread_id_mapping_by_native_id(native_tid_t);
extern native_tid_t ubench_get_native_thread_id(java_tid_t);
extern native_tid_t ubench_get_current_thread_native_id(void);
extern jint ubench_benchmark_init(void);
extern int ubench_event_init(void);
extern int ubench_event_resolve(const char*, ubench_event_info_t*);
extern void ubench_event_iterate(event_info_iterator_callback_t, void*);

extern void ubench_measure_start(const benchmark_configuration_t*, ubench_events_snapshot_t*);
extern void ubench_measure_sample(const benchmark_configuration_t*, ubench_events_snapshot_t*, int user_id);
extern void ubench_measure_stop(const benchmark_configuration_t*, ubench_events_snapshot_t*);

extern ubench_atomic_int_t counter_compilation;
extern ubench_atomic_int_t counter_compilation_total;
extern ubench_atomic_int_t counter_gc_total;

#endif
