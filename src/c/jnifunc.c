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
#pragma warning(pop)

jint JNICALL
Java_cz_cuni_mff_d3s_perf_CompilationCounter_getCompilationCountAndReset(
		JNIEnv *UNUSED_PARAMETER(env),
		jclass UNUSED_PARAMETER(unused)) {
	return ubench_atomic_uint_reset(&counter_compilation);
}
