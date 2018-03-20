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

#include "ubench.h"

#pragma warning(push, 0)
#include <string.h>
#pragma warning(pop)

#define FILL_WITH_ZEROS(variable) memset(&variable, 0, sizeof(variable))

static jvmtiEnv *agent_env;


ubench_atomic_int_t counter_compilation = { 0 };
ubench_atomic_int_t counter_compilation_total = { 0 };
ubench_atomic_int_t counter_gc_total = { 0 };

static void report_error(const char *line_prefix,
		jvmtiError error, const char *operation_description) {
	char *error_description = NULL;
	(void) (*agent_env)->GetErrorName(agent_env, error, &error_description);
	if (error_description == NULL) {
		error_description = "uknown error";
	}

	fprintf(stderr, "%sJVMTI agent error #%d: %s (%s).\n",
			line_prefix, (int) error, error_description, operation_description);
}

#define QUOTE2(x) #x
#define QUOTE(x) QUOTE2(x)

#define REPORT_ERROR(error_number, operation_description) \
	report_error("[" __FILE__ ":" QUOTE(__LINE__) "]: ", (error_number), (operation_description))

#define REGISTER_EVENT_OR_RETURN(agent_env_var, event) \
	do { \
		jvmtiError event_err = (*agent_env_var)->SetEventNotificationMode( \
			agent_env_var, JVMTI_ENABLE, event, NULL); \
		if (event_err != JVMTI_ERROR_NONE) { \
			REPORT_ERROR(err, "enabling notification for " #event); \
			return JNI_ERR; \
		} \
	} ONCE


static void JNICALL on_compiled_method_load(jvmtiEnv* UNUSED_PARAMETER(jvmti),
		jmethodID UNUSED_PARAMETER(method),
		jint UNUSED_PARAMETER(code_size),
		const void* UNUSED_PARAMETER(code_addr),
		jint UNUSED_PARAMETER(map_length),
		const jvmtiAddrLocationMap* UNUSED_PARAMETER(map),
		const void* UNUSED_PARAMETER(compile_info)) {

	/*
	 * Currently, we do not record any information about the
	 * method that was just compiled.
	 *
	 * We increment the counter only.
	 */
	ubench_atomic_int_inc(&counter_compilation);
	ubench_atomic_int_inc(&counter_compilation_total);
}

// static void JNICALL on_gc_start(jvmtiEnv *UNUSED_PARAMETER(jvmti_env)) {
// }

static void JNICALL on_gc_finish(jvmtiEnv *UNUSED_PARAMETER(jvmti_env)) {
	ubench_atomic_int_inc(&counter_gc_total);
}


static void JNICALL on_thread_start(jvmtiEnv *UNUSED_PARAMETER(jvmti_env),
		JNIEnv* jni_env, jthread thread) {
	ubench_register_this_thread(thread, jni_env);
}

static void JNICALL on_thread_end(jvmtiEnv *UNUSED_PARAMETER(jvmti_env),
		JNIEnv* jni_env, jthread thread) {
	ubench_unregister_this_thread(thread, jni_env);
}


/*
 * Register event handler for JVMTI_EVENT_COMPILED_METHOD_LOAD.
 * We need to enable the capability, set the callback and enable the
 * actual notification.
 */
static jint register_and_enable_callback(void) {
	jvmtiError err;

	jvmtiCapabilities capabilities;
	FILL_WITH_ZEROS(capabilities);
	capabilities.can_generate_compiled_method_load_events = 1;
	capabilities.can_generate_garbage_collection_events = 1;
	err = (*agent_env)->AddCapabilities(agent_env, &capabilities);
	if (err != JVMTI_ERROR_NONE) {
		REPORT_ERROR(err, "adding capabilities to intercept various JVMTI events");
		return JNI_ERR;
	}

	jvmtiEventCallbacks callbacks;
	FILL_WITH_ZEROS(callbacks);
	callbacks.CompiledMethodLoad = &on_compiled_method_load;
	// callbacks.GarbageCollectionStart = &on_gc_start;
	callbacks.GarbageCollectionFinish = &on_gc_finish;
	callbacks.ThreadStart = &on_thread_start;
	callbacks.ThreadEnd = &on_thread_end;
	err = (*agent_env)->SetEventCallbacks(agent_env, &callbacks, sizeof(callbacks));
	if (err != JVMTI_ERROR_NONE) {
		REPORT_ERROR(err, "adding callbacks for various JVMTI events");
		return JNI_ERR;
	}


	REGISTER_EVENT_OR_RETURN(agent_env, JVMTI_EVENT_COMPILED_METHOD_LOAD);
	// REGISTER_EVENT_OR_RETURN(agent_env, JVMTI_EVENT_GARBAGE_COLLECTION_START);
	REGISTER_EVENT_OR_RETURN(agent_env, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH);
	REGISTER_EVENT_OR_RETURN(agent_env, JVMTI_EVENT_THREAD_START);
	REGISTER_EVENT_OR_RETURN(agent_env, JVMTI_EVENT_THREAD_END);

	return JNI_OK;
}


jint ubench_counters_init(jvmtiEnv *jvmti) {
	DEBUG_PRINTF("initializing counters");

	agent_env = jvmti;

	jint rc;
	rc = register_and_enable_callback();
	if (rc != JNI_OK) {
		return rc;
	}

	return JNI_OK;
}

