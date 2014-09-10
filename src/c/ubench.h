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

#ifndef UBENCH_H_GUARD
#define UBENCH_H_GUARD

#pragma warning(push, 0)
#include <jvmti.h>
#include <jni.h>
#include <jvmticmlr.h>
#include <stdlib.h>
#include <string.h>
#pragma warning(pop)

#include "myatomic.h"

#ifdef __GNUC__
#define UNUSED_PARAMETER(name) name __attribute__((unused))
#else
#define UNUSED_PARAMETER(name) name
#endif

/*
 * Prevent condition expression is constant warning in wrappers around
 * multi-statement macros (the do { ... } while (0) construct).
 */
#ifdef _MSC_VER
#define ONCE \
	__pragma(warning(push)) \
	__pragma(warning(disable:4127)) \
    while (0) \
    __pragma(warning(pop))
#else
#define ONCE while (0)
#endif

#ifdef JITCOUNTER_DEBUG
#define DEBUG_PRINTF(fmt, ...) printf("[ubench-agent]: " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) (void)0
#endif

extern jint ubench_counters_init(jvmtiEnv *);
extern jint ubench_benchmark_init(void);

extern ubench_atomic_t counter_compilation;
extern ubench_atomic_t counter_compilation_total;
extern ubench_atomic_t counter_gc_total;

static inline char *ubench_str_dup(const char *str) {
	char *result = malloc(sizeof(char) * strlen(str) + 1);
	if (result == NULL) {
		return NULL;
	}

	/*
	 * We know that the buffer is big enough and thus we can use strcpy()
	 * without worrying that we would run past the buffer.
	 */
#pragma warning(suppress:4996)
	strcpy(result, str);

	return result;
}

#endif
