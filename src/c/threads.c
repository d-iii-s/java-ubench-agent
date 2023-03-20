/*
 * Copyright 2017 Charles University in Prague
 * Copyright 2017 Vojtech Horky
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
/* Ensure compatibility of JNI function types. */
#include "cz_cuni_mff_d3s_perf_NativeThreads.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#pragma warning(pop)

#ifdef __GNUC__
#pragma warning(push, 0)
#define _GNU_SOURCE
#include <sys/syscall.h>
#include <unistd.h>
#pragma warning(pop)
#endif

#ifdef __APPLE__
#pragma warning(push, 0)
#include <pthread.h>
#pragma warning(pop)
#endif


#ifdef _MSC_VER
#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)
#endif

typedef struct {
	native_tid_t native_id;
	java_tid_t java_id;
} thread_map_entry_t;

typedef struct {
	int length;
	int capacity;
	thread_map_entry_t* entries;
} thread_map_t;

typedef bool (*predicate_func_t)(const void* arg, const thread_map_entry_t* entry);

#ifdef UBENCH_DEBUG
static void
debug_thread_map_print_entries(const thread_map_t* map) {
	assert(map != NULL);

	DEBUG_PRINTF("thread_map_t(%p) = {", map);
	DEBUG_PRINTF("    .length = %d", map->length);
	DEBUG_PRINTF("    .capacity = %d", map->capacity);

	if (map->length > 0) {
		DEBUG_PRINTF("    .entries = {");

		for (int i = 0; i < map->length; i++) {
			thread_map_entry_t* entry = &map->entries[i];

			DEBUG_PRINTF(
				"       .[%d] = { .java_id = %" PRId_JAVA_TID ", .native_id = %" PRId_NATIVE_TID " }",
				i, entry->java_id, entry->native_id
			);
		};

		DEBUG_PRINTF("    }");
	} else {
		DEBUG_PRINTF("    .entries = {}");
	}

	DEBUG_PRINTF("}");
}
#else
#define debug_thread_map_print_entries(map) (void) 0
#endif

//

static inline int
thread_map_index_of(const thread_map_t* map, predicate_func_t predicate, void* arg) {
	//
	// We assume that the sequence of valid entries is contiguous.
	// Once we find an invalid entry, it means that the given thread
	// is is not present.
	//

	for (int i = 0; i < map->length; i++) {
		thread_map_entry_t* entry = &map->entries[i];
		if (predicate(arg, entry)) {
			return i;
		}
	}

	return -1;
}

//

static bool
predicate_java_id_match(const java_tid_t* java_id_arg, const thread_map_entry_t* entry) {
	java_tid_t java_thread_id = *java_id_arg;
	return entry->java_id == java_thread_id;
}

static int
thread_map_index_of_java_thread(const thread_map_t* map, java_tid_t java_thread_id) {
	return thread_map_index_of(map, (predicate_func_t) predicate_java_id_match, &java_thread_id);
}

//

static bool
predicate_native_id_match(const native_tid_t* native_id_arg, const thread_map_entry_t* entry) {
	native_tid_t native_thread_id = *native_id_arg;
	return entry->native_id == native_thread_id;
}

static int
thread_map_index_of_native_thread(const thread_map_t* map, native_tid_t native_thread_id) {
	return thread_map_index_of(map, (predicate_func_t) predicate_native_id_match, &native_thread_id);
}

//

static bool
thread_map_ensure_capacity(thread_map_t* map, int required_capacity) {
	assert(map != NULL);

	if (map->capacity < required_capacity) {
		thread_map_entry_t* new_entries = realloc(map->entries, required_capacity * sizeof(thread_map_entry_t));
		if (new_entries == NULL) {
			return false;
		}

		map->entries = new_entries;
		map->capacity = required_capacity;
	}

	return true;
}

static int
thread_map_put_java_thread(thread_map_t* map, java_tid_t java_thread_id, native_tid_t native_thread_id) {
	assert(map != NULL);

	// Lookup the entry and if it exists, indicate that nothing was added.
	int index = thread_map_index_of_java_thread(map, java_thread_id);
	if (index >= 0) {
		return EEXIST;
	}

	// Ensure there is enough room to add a new entry.
	if (!thread_map_ensure_capacity(map, map->length + 1)) {
		return ENOMEM;
	}

	// Fill the entry and indicate that a new entry was added.
	map->entries[map->length] = (thread_map_entry_t) {
		.java_id = java_thread_id,
		.native_id = native_thread_id,
	};

	map->length++;
	return 0;
}

//

static int
thread_map_remove_native_thread(thread_map_t* map, native_tid_t native_thread_id) {
	assert(map != NULL);

	//
	// If the entry exists, replace it with the last entry and shrink the map by one.
	//
	int index = thread_map_index_of_native_thread(map, native_thread_id);
	if (index < 0) {
		return ENOENT;
	}

	// The entry exists, so the map must have a non-zero length.
	assert(map->length > 0);
	map->length--;

	// The new length is the index of the last element.
	map->entries[index] = map->entries[map->length];
	return 0;
}

//

static thread_map_t thread_map;
static ubench_spinlock_t thread_map_lock = UBENCH_SPINLOCK_INITIALIZER;

static bool
ubench_register_java_thread(java_tid_t java_thread_id, native_tid_t native_thread_id) {
	ubench_spinlock_lock(&thread_map_lock);

	int result = thread_map_put_java_thread(&thread_map, java_thread_id, native_thread_id);
	debug_thread_map_print_entries(&thread_map);

	ubench_spinlock_unlock(&thread_map_lock);

	if (result == ENOMEM) {
		// TODO Consider throwing an exception from the caller.
		fprintf(stderr, "fatal: failed to register Java thread with native id, aborting!\n");
		exit(1);
	}

	return (result == 0);
}

static bool
ubench_unregister_native_thread(native_tid_t native_thread_id) {
	ubench_spinlock_lock(&thread_map_lock);

	int result = thread_map_remove_native_thread(&thread_map, native_thread_id);
	debug_thread_map_print_entries(&thread_map);

	ubench_spinlock_unlock(&thread_map_lock);
	return (result == 0);
}

//

static ubench_spinlock_t java_lang_Thread_guard = UBENCH_SPINLOCK_INITIALIZER;
static jclass class_java_lang_Thread = NULL;
static jmethodID method_java_lang_Thread_getId = NULL;

static void
ensure_java_lang_Thread_resolved(JNIEnv* jni_env) {
	ubench_spinlock_lock(&java_lang_Thread_guard);

	if (class_java_lang_Thread != NULL) {
		ubench_spinlock_unlock(&java_lang_Thread_guard);
		return;
	}

	class_java_lang_Thread = (*jni_env)->FindClass(jni_env, "java/lang/Thread");
	if (class_java_lang_Thread == NULL) {
		fprintf(stderr, "Failed to find java.lang.Thread, aborting!\n");
		exit(1);
	}
	method_java_lang_Thread_getId = (*jni_env)->GetMethodID(jni_env, class_java_lang_Thread, "getId", "()J");
	if (method_java_lang_Thread_getId == NULL) {
		fprintf(stderr, "Failed to find java.lang.Thread.getId(), aborting!\n");
		exit(1);
	}

	ubench_spinlock_unlock(&java_lang_Thread_guard);
}

INTERNAL void JNICALL
ubench_jvm_callback_on_thread_start(
	jvmtiEnv* UNUSED_PARAMETER(jvmti), JNIEnv* jni, jthread thread
) {
	ensure_java_lang_Thread_resolved(jni);

	java_tid_t java_id = (*jni)->CallLongMethod(jni, thread, method_java_lang_Thread_getId);
	native_tid_t native_id = ubench_get_current_thread_native_id();

	DEBUG_PRINTF("JVM callback: thread %" PRId_JAVA_TID " [%" PRId_NATIVE_TID "] started.", java_id, native_id);

	ubench_register_java_thread(java_id, native_id);
}

INTERNAL void JNICALL
ubench_jvm_callback_on_thread_end(
	jvmtiEnv* UNUSED_PARAMETER(jvmti), JNIEnv* UNUSED_PARAMETER(jni),
	jthread UNUSED_PARAMETER(thread)
) {
	native_tid_t native_id = ubench_get_current_thread_native_id();

	DEBUG_PRINTF("JVM callback: thread [%" PRId_NATIVE_TID "] ended.", native_id);

	ubench_unregister_native_thread(native_id);
}


INTERNAL native_tid_t
ubench_get_current_thread_native_id(void) {
#if defined(_MSC_VER)
	return (native_tid_t) GetCurrentThreadId();
#elif defined(__APPLE__)
	pthread_t tid = pthread_self();
	return (native_tid_t) tid;
#elif defined(__GNUC__)
	pid_t answer = syscall(__NR_gettid);
	return (native_tid_t) answer;
#else
#error "Threading not supported on this platform."
	return UBENCH_THREAD_ID_INVALID;
#endif
}

INTERNAL native_tid_t
ubench_threads_get_native_id(java_tid_t java_thread_id) {
	ubench_spinlock_lock(&thread_map_lock);

	int index = thread_map_index_of_java_thread(&thread_map, java_thread_id);
	native_tid_t result = (index >= 0) ? thread_map.entries[index].native_id : UBENCH_THREAD_ID_INVALID;

	ubench_spinlock_unlock(&thread_map_lock);
	return result;
}

//

JNIEXPORT java_tid_t JNICALL
Java_cz_cuni_mff_d3s_perf_NativeThreads_getNativeId(
	JNIEnv* UNUSED_PARAMETER(jni), jclass UNUSED_PARAMETER(threads_class),
	java_tid_t java_thread_id
) {
	native_tid_t native_thread_id = ubench_threads_get_native_id(java_thread_id);
	if (native_thread_id == UBENCH_THREAD_ID_INVALID) {
		// TODO Consider throwing an exception (-1 could be a valid id).
		return (java_tid_t) cz_cuni_mff_d3s_perf_NativeThreads_INVALID_THREAD_ID;
	}

	return (java_tid_t) native_thread_id;
}

JNIEXPORT jboolean JNICALL
Java_cz_cuni_mff_d3s_perf_NativeThreads_registerJavaThread(
	JNIEnv* UNUSED_PARAMETER(jni), jclass UNUSED_PARAMETER(threads_class),
	java_tid_t java_thread_id, java_tid_t jnative_thread_id
) {
	// TODO Consider throwing an exception ('false' really means "already registered").
	return ubench_register_java_thread(java_thread_id, (native_tid_t) jnative_thread_id);
}
