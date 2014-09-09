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


#include "microbench.h"
#pragma warning(push, 0)
#include "cz_cuni_mff_d3s_perf_OverheadEstimations.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef HAS_GETRUSAGE
#include <sys/resource.h>
#endif
#pragma warning(pop)

void JNICALL Java_cz_cuni_mff_d3s_perf_OverheadEstimations_emptyNativeCall(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	return;
}


void JNICALL Java_cz_cuni_mff_d3s_perf_OverheadEstimations_resourceUsageCall(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
#ifdef HAS_GETRUSAGE
	struct rusage resource_usage;
	getrusage(RUSAGE_SELF, &resource_usage);
#endif
}

