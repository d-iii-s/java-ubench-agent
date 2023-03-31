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

#ifndef JVMUTIL_H_GUARD
#define JVMUTIL_H_GUARD

#pragma warning(push, 0)
#include <stdbool.h>

#include <jni.h>
#include <jvmti.h>
#pragma warning(pop)

typedef struct jvmti_context {
	jvmtiEnv* jvmti;

	bool has_capabilities;
	jvmtiCapabilities capabilities;

	jvmtiEventCallbacks callbacks;

	// Zero-terminated array of JVMTI events.
	// The zero element MUST be always present!
	jvmtiEvent events[];
} jvmti_context_t;

extern bool ubench_jvmti_context_init(jvmti_context_t*, JavaVM*);
extern bool ubench_jvmti_context_enable(jvmti_context_t*);
extern bool ubench_jvmti_context_destroy(jvmti_context_t*);
extern bool ubench_jvmti_context_init_and_enable(jvmti_context_t*, JavaVM*);

#endif

