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

#pragma warning(push, 0)
/* Ensure compatibility of JNI function types. */
#include "cz_cuni_mff_d3s_perf_OverheadEstimations.h"

#ifdef HAS_GETRUSAGE
#include <sys/resource.h>
#endif

#include <jni.h>
#pragma warning(pop)

JNIEXPORT void JNICALL
Java_cz_cuni_mff_d3s_perf_OverheadEstimations_emptyNativeCall(
	JNIEnv* UNUSED_PARAMETER(jni), jclass UNUSED_PARAMETER(overhead_class)
) {
	return;
}

JNIEXPORT void JNICALL
Java_cz_cuni_mff_d3s_perf_OverheadEstimations_resourceUsageCall(
	JNIEnv* UNUSED_PARAMETER(jni), jclass UNUSED_PARAMETER(overhead_class)
) {
#ifdef HAS_GETRUSAGE
	struct rusage resource_usage;
	getrusage(RUSAGE_SELF, &resource_usage);
#endif
}
