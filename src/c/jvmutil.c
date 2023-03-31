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
#include "jvmutil.h"
#include "logging.h"

#pragma warning(push, 0)
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <jni.h>
#include <jvmti.h>
#pragma warning(pop)

#ifdef UBENCH_DEBUG
static void
debug_ubench_jvmti_context_print(jvmti_context_t* context, const char* name) {
	DEBUG_PRINTF("%s(%p) = {", (name != NULL) ? name : "jvmti_context_t", context);
	DEBUG_PRINTF("    .jvmti = %p", context->jvmti);

	jvmtiEventReserved* callbacks = (jvmtiEventReserved*) &context->callbacks;
	for (size_t i = 0; i < sizeof(jvmtiEventCallbacks) / sizeof(*callbacks); i++) {
		if (callbacks[i] != 0) {
			DEBUG_PRINTF("    .callbacks[%02zu] = %p", i, callbacks[i]);
		}
	}

	for (int i = 0; context->events[i] != 0; i++) {
		DEBUG_PRINTF("    .events[%02d] = %d", i, context->events[i]);
	}

	DEBUG_PRINTF("    .has_capabilities = %s", context->has_capabilities ? "yes" : "no");
	uint32_t* caps = (uint32_t*) &context->capabilities;
	for (size_t i = 0; i < sizeof(jvmtiCapabilities) / sizeof(*caps); i++) {
		DEBUG_PRINTF(
			"    .capabilities[%03zu:%03zu] = %08x",
			((i + 1) * 8 * sizeof(*caps) - 1), i * 8 * sizeof(*caps), caps[i]
		);
	}

	DEBUG_PRINTF("}");
}
#else
#define debug_ubench_jvmti_context_print(context, name) (void) 0
#endif

INTERNAL bool
ubench_jvmti_context_init(jvmti_context_t* context, JavaVM* jvm) {
	assert(context != NULL && jvm != NULL);

	jvmtiEnv* jvmti;

	debug_ubench_jvmti_context_print(context, NULL);

	// Create a new JVMTI environment and store it in the context if successful.
	jint env_error = (*jvm)->GetEnv(jvm, (void**) &jvmti, JVMTI_VERSION_1_2);
	if (env_error != JNI_OK) {
		DEBUG_PRINTF("failed to get JVMTI environment (error %ld).", (long) env_error);
		return false;
	}

	context->jvmti = jvmti;
	return true;
}

INTERNAL bool
ubench_jvmti_context_enable(jvmti_context_t* context) {
	assert(context != NULL && context->jvmti != NULL);

	jvmtiEnv* jvmti = context->jvmti;

	// Add capabilities required to enable certain events.
	if (context->has_capabilities) {
		jvmtiError cap_error = (*jvmti)->AddCapabilities(jvmti, &context->capabilities);
		if (cap_error != JVMTI_ERROR_NONE) {
			DEBUG_PRINTF("failed to add capabilities (error %ld).", (long) cap_error);
			return false;
		}
	}


	// Enable events first, because we need to do it one by one.
	// To actually generate events, an event must have a callback.
	for (int i = 0; context->events[i] != 0; i++) {
		jvmtiEvent event = context->events[i];
		jvmtiError event_err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, event, NULL);
		if (event_err != JVMTI_ERROR_NONE) {
			DEBUG_PRINTF("failed to enable JVMTI event %ld (error %ld).", (long) event, (long) event_err);
			return false;
		}
	}


	// Register callbacks last, only after we enable all events.
	// This is atomic and allows actual events to be generated.
	jvmtiError cb_error = (*jvmti)->SetEventCallbacks(jvmti, &context->callbacks, sizeof(context->callbacks));
	if (cb_error != JVMTI_ERROR_NONE) {
		DEBUG_PRINTF("failed to set event callbacks (error %ld).", (long) cb_error);
		return false;
	}

	return true;
}

INTERNAL bool
ubench_jvmti_context_init_and_enable(jvmti_context_t* context, JavaVM* jvm) {
	assert(context != NULL && jvm != NULL);

	if (ubench_jvmti_context_init(context, jvm)) {
		if (ubench_jvmti_context_enable(context)) {
			return true;
		}

		ubench_jvmti_context_destroy(context);
	}

	return false;
}

INTERNAL bool
ubench_jvmti_context_destroy(jvmti_context_t* context) {
	assert(context != NULL);

	jvmtiEnv* jvmti = context->jvmti;
	if (jvmti != NULL) {
		context->jvmti = NULL;

		jvmtiError dispose_error = (*jvmti)->DisposeEnvironment(jvmti);
		if (dispose_error == JVMTI_ERROR_NONE) {
			return true;
		}

		DEBUG_PRINTF("failed to dispose JVMTI environment (error %ld).", (long) dispose_error);

	} else {
		DEBUG_PRINTF("JVMTI context does not have a valid JVMTI environment.");
	}

	return false;
}
