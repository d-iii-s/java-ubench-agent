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

#include "compiler.h"
#include "logging.h"
#include "jvmutil.h"
#include "myatomic.h"
#include "mylock.h"
#include "ubench.h"

#pragma warning(push, 0)
/* Ensure compatibility of JNI function types. */
#include "cz_cuni_mff_d3s_perf_NativeThreads.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <jni.h>
#include <jvmti.h>
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
debug_thread_map_print_entries(const thread_map_t* map, const char* prefix) {
	assert(map != NULL);

	DEBUG_PRINTF("%sthread_map_t(%p) = {", (prefix != NULL) ? prefix : "", map);
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
#define debug_thread_map_print_entries(map, prefix) (void) 0
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
			DEBUG_PRINTF("failed to reallocate memory for thread map entries.");
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
		DEBUG_PRINTF("failed to expand thread map capacity.");
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
		DEBUG_PRINTF("native thread with id %" PRId_NATIVE_TID " does not exist.", native_thread_id);
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

	debug_thread_map_print_entries(&thread_map, "before adding: ");
	int result = thread_map_put_java_thread(&thread_map, java_thread_id, native_thread_id);
	debug_thread_map_print_entries(&thread_map, "after adding: ");

	ubench_spinlock_unlock(&thread_map_lock);

	if (result == ENOMEM) {
		// TODO Consider throwing an exception from the caller.
		FATAL_PRINTF("failed to register Java thread with native id, aborting!");
		exit(1);
	}

	return (result == 0);
}

static bool
ubench_unregister_native_thread(native_tid_t native_thread_id) {
	ubench_spinlock_lock(&thread_map_lock);

	debug_thread_map_print_entries(&thread_map, "before removal: ");
	int result = thread_map_remove_native_thread(&thread_map, native_thread_id);
	debug_thread_map_print_entries(&thread_map, "after removal: ");

	ubench_spinlock_unlock(&thread_map_lock);
	return (result == 0);
}

//

static inline native_tid_t
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

//

/*
 * Cached identifier of 'java.lang.Thread.getId()' method.
 * Initialized from the 'VM Init' callback, which is called before the
 * main thread is started. Allows the 'Thread Start' and 'Thread End'
 * callbacks get the Java thread identifier.
 */
static jmethodID thread_get_id_method;

static void JNICALL
jvmti_callback_on_thread_start(
	jvmtiEnv* UNUSED_PARAMETER(jvmti), JNIEnv* jni, jthread thread
) {
#ifdef UBENCH_DEBUG
	jvmtiThreadInfo thread_info = (jvmtiThreadInfo) { .name = NULL, /* clear the structure */ };

	jvmtiError ti_error = (*jvmti)->GetThreadInfo(jvmti, thread, &thread_info);
	assert(ti_error == JVMTI_ERROR_NONE);
#endif

	DEBUG_PRINTF("thread %p [%s] started.", thread, thread_info.name);

	java_tid_t java_id = (*jni)->CallLongMethod(jni, thread, thread_get_id_method);
	native_tid_t native_id = ubench_get_current_thread_native_id();
	bool registered = ubench_register_java_thread(java_id, native_id);

	DEBUG_PRINTF(
		"%s thread %p [%s, %" PRId_JAVA_TID "] with native id [%" PRId_NATIVE_TID "].",
		registered ? "registered" : "failed to register",
		thread, thread_info.name, java_id, native_id
	);

	UNUSED_VARIABLE(registered);

#ifdef UBENCH_DEBUG
	if (thread_info.name != NULL) {
		ti_error = (*jvmti)->Deallocate(jvmti, (unsigned char*) thread_info.name);
		assert(ti_error == JVMTI_ERROR_NONE);
	}
#endif
}

static void JNICALL
jvmti_callback_on_thread_end(
	jvmtiEnv* UNUSED_PARAMETER(jvmti), JNIEnv* UNUSED_PARAMETER(jni),
	jthread UNUSED_PARAMETER(thread)
) {
#ifdef UBENCH_DEBUG
	jvmtiThreadInfo thread_info = (jvmtiThreadInfo) { .name = NULL, /* clear the structure */ };

	jvmtiError ti_error = (*jvmti)->GetThreadInfo(jvmti, thread, &thread_info);
	assert(ti_error == JVMTI_ERROR_NONE);
#endif

	DEBUG_PRINTF("thread %p [%s] finished.", thread, thread_info.name);

	native_tid_t native_id = ubench_get_current_thread_native_id();
	bool unregistered = ubench_unregister_native_thread(native_id);

	DEBUG_PRINTF(
		"%s thread %p [%s] with native id [%" PRId_NATIVE_TID "].",
		unregistered ? "unregistered" : "failed to unregister",
		thread, thread_info.name, native_id
	);

	UNUSED_VARIABLE(unregistered);

#ifdef UBENCH_DEBUG
	if (thread_info.name != NULL) {
		ti_error = (*jvmti)->Deallocate(jvmti, (unsigned char*) thread_info.name);
		assert(ti_error == JVMTI_ERROR_NONE);
	}
#endif
}

static jvmti_context_t threads_context = {
	.callbacks = {
		.ThreadStart = &jvmti_callback_on_thread_start,
		.ThreadEnd = &jvmti_callback_on_thread_end,
	},
	.events = { JVMTI_EVENT_THREAD_START, JVMTI_EVENT_THREAD_END, 0 }
};

//

static void JNICALL
jvmti_callback_on_vm_init(
	jvmtiEnv* UNUSED_PARAMETER(jvmti), JNIEnv* jni, jthread UNUSED_PARAMETER(thread)
) {
	//
	// Initialize the 'thread_class' and 'thread_get_id_method' variables for
	// the thread registration callbacks.
	//
	jclass thread_class = (*jni)->FindClass(jni, "java/lang/Thread");
	if (thread_class == NULL) {
		FATAL_PRINTF("failed to find 'java.lang.Thread' class, aborting!");
		exit(1);
	}

	thread_get_id_method = (*jni)->GetMethodID(jni, thread_class, "getId", "()J");
	if (thread_get_id_method == NULL) {
		FATAL_PRINTF("failed to find 'java.lang.Thread.getId()' method, aborting!\n");
		exit(1);
	}

	//
	// Enable the thread registration JVMTI context to automatically register
	// newly created threads. This ensures that the thread registration methods
	// will be called only after the variables have been initialized.
	//
	if (!ubench_jvmti_context_enable(&threads_context)) {
		DEBUG_PRINTF("could not enable thread registration JVMTI context");
	}
}

INTERNAL bool
ubench_threads_init(JavaVM* jvm) {
	assert(jvm != NULL);

	static jvmti_context_t init_context = {
		.callbacks = { .VMInit = &jvmti_callback_on_vm_init },
		.events = { JVMTI_EVENT_VM_INIT, 0 }
	};

	//
	// Initialize the thread registration context. If it succeeds, initialize
	// and enable the VM initialization context, which will enable the thread
	// registration context upon receiving the 'VM Init' event.
	//
	if (ubench_jvmti_context_init(&threads_context, jvm)) {
		if (ubench_jvmti_context_init_and_enable(&init_context, jvm)) {
			return true;
		}

		ubench_jvmti_context_destroy(&threads_context);
	}

	return false;
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

JNIEXPORT jboolean JNICALL
Java_cz_cuni_mff_d3s_perf_NativeThreads_registerCurrentJavaThread(
	JNIEnv* UNUSED_PARAMETER(jni), jclass UNUSED_PARAMETER(threads_class),
	java_tid_t java_thread_id
) {
	// TODO Consider throwing an exception ('false' really means "already registered").
	return ubench_register_java_thread(java_thread_id, ubench_get_current_thread_native_id());
}
