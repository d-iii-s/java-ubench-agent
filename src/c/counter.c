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

#include <string.h>

#include "microbench.h"

#define FILL_WITH_ZEROS(variable) memset(&variable, 0, sizeof(variable))

static jvmtiEnv *agent_env;

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

static void JNICALL
compilation_counter_on_compiled_method_load(jvmtiEnv* UNUSED_PARAMETER(jvmti),
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
	ubench_atomic_add(&counter_compilation, 1);
	ubench_atomic_add(&counter_compilation_total, 1);
}

/*
 * Register event handler for JVMTI_EVENT_COMPILED_METHOD_LOAD.
 * We need to enable the capability, set the callback and enable the
 * actual notification.
 */
static jint register_and_enable_callback() {
	jvmtiError err;

	jvmtiCapabilities capabilities;
	FILL_WITH_ZEROS(capabilities);
	capabilities.can_generate_compiled_method_load_events = 1;
	err = (*agent_env)->AddCapabilities(agent_env, &capabilities);
	if (err != JVMTI_ERROR_NONE) {
		REPORT_ERROR(err, "adding capability to intercept JVMTI_EVENT_COMPILED_METHOD_LOAD");
		return JNI_ERR;
	}

	jvmtiEventCallbacks callbacks;
	FILL_WITH_ZEROS(callbacks);
	callbacks.CompiledMethodLoad = &compilation_counter_on_compiled_method_load;
	err = (*agent_env)->SetEventCallbacks(agent_env, &callbacks, sizeof(callbacks));
	if (err != JVMTI_ERROR_NONE) {
		REPORT_ERROR(err, "adding callback for JVMTI_EVENT_COMPILED_METHOD_LOAD");
		return JNI_ERR;
	}

	err = (*agent_env)->SetEventNotificationMode(agent_env, JVMTI_ENABLE,
			JVMTI_EVENT_COMPILED_METHOD_LOAD, NULL);
	if (err != JVMTI_ERROR_NONE) {
		REPORT_ERROR(err, "enabling notification for JVMTI_EVENT_COMPILED_METHOD_LOAD");
		return JNI_ERR;
	}

	return JNI_OK;
}

ubench_atomic_t counter_compilation;
ubench_atomic_t counter_compilation_total;

static void prepare_counters() {
	ubench_atomic_init(&counter_compilation, 0);
	ubench_atomic_init(&counter_compilation_total, 0);
}


jint ubench_counters_init(jvmtiEnv *jvmti) {
	DEBUG_PRINTF("initializing counters");

	agent_env = jvmti;

	jint rc;

	rc = register_and_enable_callback();
	if (rc != JNI_OK) {
		return rc;
	}

	prepare_counters();

	return JNI_OK;
}

