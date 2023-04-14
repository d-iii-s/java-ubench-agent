/*
 * Copyright 2023 Charles University in Prague
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

	jvmtiEventCallbacks callbacks;

	// Zero-terminated array of JVMTI events.
	// The zero element MUST be always present!
	jvmtiEvent events[(JVMTI_MAX_EVENT_TYPE_VAL - JVMTI_MIN_EVENT_TYPE_VAL + 1) + 1];

	bool has_capabilities;

	// Keep this last. When using aggregate initializers, MSVC 19.0 does not
	// calculate the size of the bitfield properly, which results in the values
	// of the subsequent fields to be shifted (into the bitfield).
	//
	// This is why we also keep a safety pad at the end.
	jvmtiCapabilities capabilities;

	unsigned long __padding;
} jvmti_context_t;

extern bool ubench_jvmti_context_init(jvmti_context_t*, JavaVM*);
extern bool ubench_jvmti_context_enable(jvmti_context_t*);
extern bool ubench_jvmti_context_destroy(jvmti_context_t*);
extern bool ubench_jvmti_context_init_and_enable(jvmti_context_t*, JavaVM*);

#endif

