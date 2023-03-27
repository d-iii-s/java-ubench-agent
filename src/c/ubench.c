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
#include "myatomic.h"
#include "ubench.h"

#pragma warning(push, 0)
/* Ensure compatibility of JNI function types. */
#include "cz_cuni_mff_d3s_perf_UbenchAgent.h"

#include <assert.h>
#include <stdbool.h>

#include <jni.h>
#include <jvmti.h>
#pragma warning(pop)

//

static bool
ubench_startup(JavaVM* jvm) {
	assert(jvm != NULL);

	//
	// Avoid initializing the library twice.
	//
	// This could happen if the library is loaded as a JVM agent and also
	// explicitly loaded using the 'loadLibrary()' call. We rely on the
	// 'UbenchAgent' class to avoid loading the library if it was already
	// loaded as an agent, so this is just an extra precaution.
	//
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

/**
 * Dummy function used by the 'UbenchAgent' class to check if the native
 * functions are already bound. If the call to this function succeeds, it
 * means that the agent has been already loaded and there is no need to
 * try loading it via the 'loadLibrary()' method which could fail if the
 * agent was loaded using the '-agentlib' JVM option without setting the
 * value of the 'java.library.path'.
 */
JNIEXPORT jboolean JNICALL
Java_cz_cuni_mff_d3s_perf_UbenchAgent_nativeIsLoaded(
	JNIEnv* UNUSED_PARAMETER(jni), jclass UNUSED_PARAMETER(ubench_class)
) {
	DEBUG_PRINTF("nativeIsLoaded called from Java");
	return JNI_TRUE;
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
