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
#include <stdio.h>
#include <string.h>
#pragma warning(pop)

JNIEXPORT jint JNICALL
Agent_OnLoad(
	JavaVM* jvm, char* UNUSED_PARAMETER(options), void* UNUSED_PARAMETER(reserved)
) {
	jint rc;
	jvmtiEnv* jvmti;

	rc = (*jvm)->GetEnv(jvm, (void**) &jvmti, JVMTI_VERSION);
	if (rc != JNI_OK) {
		fprintf(stderr, "Unable to create JVMTI environment, JavaVM->GetEnv failed, error %ld.\n", (long) rc);
		return JNI_ERR;
	}

	rc = ubench_counters_init(jvmti);

	if (rc != JNI_OK) {
		return JNI_ERR;
	}

	rc = ubench_benchmark_init();
	if (rc != JNI_OK) {
		return JNI_ERR;
	}

	DEBUG_PRINTF("initialization completed successfully.");
	return JNI_OK;
}

JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM* UNUSED_PARAMETER(jvm)) {
	DEBUG_PRINTF("agent unloaded.");
}