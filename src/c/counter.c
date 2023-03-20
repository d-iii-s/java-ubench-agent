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
#include "myatomic.h"
#include "ubench.h"

#pragma warning(push, 0)
/* Ensure compatibility of JNI function types. */
#include "cz_cuni_mff_d3s_perf_CompilationCounter.h"

#include <assert.h>
#include <stdbool.h>

#include <jni.h>
#include <jvmti.h>
#include <jvmticmlr.h>
#pragma warning(pop)


ubench_atomic_int_t counter_compilation = { 0 };
ubench_atomic_int_t counter_compilation_total = { 0 };
ubench_atomic_int_t counter_gc_total = { 0 };

static void JNICALL
jvmti_callback_on_compiled_method_load(
	jvmtiEnv* UNUSED_PARAMETER(jvmti), jmethodID UNUSED_PARAMETER(method),
	jint UNUSED_PARAMETER(code_size), const void* UNUSED_PARAMETER(code_addr),
	jint UNUSED_PARAMETER(map_length), const jvmtiAddrLocationMap* UNUSED_PARAMETER(map),
	const void* UNUSED_PARAMETER(compile_info)
) {
	/*
	 * Currently, we do not record any information about the
	 * method that was just compiled.
	 *
	 * We increment the counter only.
	 */
	ubench_atomic_int_inc(&counter_compilation);
	ubench_atomic_int_inc(&counter_compilation_total);
}

static void JNICALL
jvmti_callback_on_garbage_collection_finish(jvmtiEnv* UNUSED_PARAMETER(jvmti)) {
	ubench_atomic_int_inc(&counter_gc_total);
}

static jvmti_context_t counters_context = {
	.has_capabilities = true,
	.capabilities = {
		.can_generate_compiled_method_load_events = 1,
		.can_generate_garbage_collection_events = 1,
	},
	.callbacks = {
		.CompiledMethodLoad = &jvmti_callback_on_compiled_method_load,
		.GarbageCollectionFinish = &jvmti_callback_on_garbage_collection_finish,
	},
	.events = { JVMTI_EVENT_COMPILED_METHOD_LOAD, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, 0 }
};

//

INTERNAL bool
ubench_counters_init(JavaVM* jvm) {
	assert(jvm != NULL);
	return ubench_jvmti_context_init_and_enable(&counters_context, jvm);
}

//

JNIEXPORT jint JNICALL
Java_cz_cuni_mff_d3s_perf_CompilationCounter_getCompilationCountAndReset(
	JNIEnv* UNUSED_PARAMETER(jni), jclass UNUSED_PARAMETER(counter_class)
) {
	return ubench_atomic_int_reset(&counter_compilation);
}
