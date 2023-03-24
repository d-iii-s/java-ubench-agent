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

#include "compiler.h"
#include "logging.h"
#include "ubench.h"

#pragma warning(push, 0)
#include <assert.h>
#include <stdbool.h>

#include <jni.h>
#include <jvmti.h>
#pragma warning(pop)

//

static void JNICALL
jvmti_callback_on_vm_init(
	jvmtiEnv* UNUSED_PARAMETER(jvmti), JNIEnv* UNUSED_PARAMETER(jni),
	jthread UNUSED_PARAMETER(thread)
) {
	//
	// Set the static boolean field 'alreadyLoaded' in the 'UbenchAgent' class to 'true'
	// to indicate that native methods are already bound, because the library has been
	// already loaded through the agent mechanism. This will prevent 'UbenchAgent' from
	// trying to load the library explicitly. Even though the library can handle it, the
	// 'loadLibrary()' call could fail if the 'java.library.path' is not set correctly.
	//
	jclass loader_class = (*jni)->FindClass(jni, "cz/cuni/mff/d3s/perf/UbenchAgent");
	if (loader_class == NULL) {
		WARN_PRINTF("could not find 'UbenchAgent' class.");
		return;
	}

	jfieldID field_id = (*jni)->GetStaticFieldID(jni, loader_class, "alreadyLoaded", "Z");
	if (field_id == NULL) {
		WARN_PRINTF("could not find 'alreadyLoaded' field in the 'UbenchAgent' class.");
		return;
	}

	(*jni)->SetStaticBooleanField(jni, loader_class, field_id, JNI_TRUE);
}

static jvmti_context_t init_context = {
	.callbacks = { .VMInit = &jvmti_callback_on_vm_init },
	.events = { JVMTI_EVENT_VM_INIT, 0 }
};

static bool
ubench_signal_agent_loaded_on_vm_init(JavaVM* jvm) {
	assert(jvm != NULL);
	return ubench_jvmti_context_init_and_enable(&init_context, jvm);
}

//

static bool
ubench_startup(JavaVM* jvm) {
	assert(jvm != NULL);

	// Avoid initializing the library twice on startup.
	static ubench_atomic_int_t startup_guard = { .atomic_value = 0 };

	bool initialized = ubench_atomic_int_inc(&startup_guard) != 0;
	if (initialized) {
		DEBUG_PRINTF("ubench already initialized.");
		return true;
	}

	//
	// Initialize the individual modules. Some of them are allowed to fail,
	// because they will try to use JVMTI which may be unavailable (e.g., in
	// the native image). This will make some functionality unavailable, but
	// will still allow using the agent e.g., to use PAPI.
	//
	DEBUG_PRINTF("initializing counters module");
	if (!ubench_counters_init(jvm)) {
		WARN_PRINTF("failed to initialize event counter module.");
	}

	DEBUG_PRINTF("initializing threads module");
	if (!ubench_threads_init(jvm)) {
		WARN_PRINTF("failed to initialize threads module.");
	}

	DEBUG_PRINTF("initializing measurement module");
	if (!ubench_measurement_init()) {
		ERROR_PRINTF("failed to initialize measurement module.");
		return false;
	}

	DEBUG_PRINTF("initializing event module");
	if (!ubench_event_init()) {
		ERROR_PRINTF("failed to initialize event module.");
		return false;
	}

	// When the VM is initialized, signal that the agent has been loaded.
	// This will not work in an environment without JVMTI (e.g., native image),
	// which means that the library must have been loaded explicitly.
	if (!ubench_signal_agent_loaded_on_vm_init(jvm)) {
		DEBUG_PRINTF("unable to signal that the agent has been loaded.");
	}

	return true;
}

static void
ubench_shutdown(JavaVM* UNUSED_PARAMETER(jvm)) {
	assert(jvm != NULL);

	// Avoid finalizing the library twice on shutdown.
	static ubench_atomic_int_t shutdown_guard = { .atomic_value = 0 };

	bool finalized = ubench_atomic_int_inc(&shutdown_guard) != 0;
	if (finalized) {
		DEBUG_PRINTF("ubench already finalized.");
		return;
	}

	// So far, there is nothing to do on shutdown.
	// We do not dispose the JVMTI environments.
}

//

JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM* vm, char* UNUSED_PARAMETER(options), void* UNUSED_PARAMETER(reserved)) {
	DEBUG_PRINTF("agent loading started.");
	bool success = ubench_startup(vm);
	DEBUG_PRINTF("agent loading finished (%s).", success ? "success" : "failure");
	return success ? JNI_OK : JNI_ERR;
}

JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM* vm) {
	DEBUG_PRINTF("agent unloading started.");
	ubench_shutdown(vm);
	DEBUG_PRINTF("agent unloading finished.");
}

//

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* jvm, void* UNUSED_PARAMETER(reserved)) {
	DEBUG_PRINTF("library loading started.");
	bool success = ubench_startup(jvm);
	DEBUG_PRINTF("library loading finished (%s).", success ? "success" : "failure");
	return success ? JNI_VERSION_1_8 : 0;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM* jvm, void* UNUSED_PARAMETER(reserved)) {
	DEBUG_PRINTF("library unloading started.");
	ubench_shutdown(jvm);
	DEBUG_PRINTF("library unloading finished.");
}
