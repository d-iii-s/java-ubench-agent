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
#include "cz_cuni_mff_d3s_perf_CompilationCounter.h"
#include "cz_cuni_mff_d3s_perf_NativeThreads.h"
#pragma warning(pop)

DLL_EXPORT jint JNICALL
Java_cz_cuni_mff_d3s_perf_CompilationCounter_getCompilationCountAndReset(
	JNIEnv* UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(unused)
) {
	return ubench_atomic_int_reset(&counter_compilation);
}

JNIEXPORT jlong JNICALL
Java_cz_cuni_mff_d3s_perf_NativeThreads_getNativeId(
	JNIEnv* UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(unused),
	jlong java_thread_id
) {
	long long answer = ubench_get_native_thread_id((long) java_thread_id);
	if (answer == UBENCH_THREAD_ID_INVALID) {
		// TODO: throw an error
		return (jlong) cz_cuni_mff_d3s_perf_NativeThreads_INVALID_THREAD_ID;
	}

	return (jlong) answer;
}

JNIEXPORT jboolean JNICALL
Java_cz_cuni_mff_d3s_perf_NativeThreads_registerJavaThread(
	JNIEnv* UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(unused),
	jlong java_thread_id, jlong native_thread_id
) {
	int res = ubench_register_thread_id_mapping((long) java_thread_id, (long long) native_thread_id);
	return res == 0;
}
