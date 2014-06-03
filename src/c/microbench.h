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

#include <jvmti.h>
#include <jni.h>
#include <jvmticmlr.h>
#include "myatomic.h"

#ifdef __GNUC__
#define UNUSED_PARAMETER(name) name __attribute__((unused))
#else
#define UNUSED_PARAMETER(name) name
#endif

#ifdef JITCOUNTER_DEBUG
#define DEBUG_PRINTF(fmt, ...) printf("[ubench-agent]: " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) (void)0
#endif

extern jint ubench_counters_init(jvmtiEnv *);

extern ubench_atomic_t counter_compilation;
extern ubench_atomic_t counter_compilation_total;
extern ubench_atomic_t counter_gc_total;
