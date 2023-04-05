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

#ifndef MYATOMIC_H_GUARD
#define MYATOMIC_H_GUARD

#include "compiler.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)
#endif

typedef struct {
#ifdef _MSC_VER
	LONG atomic_value;
#else
	int atomic_value;
#endif
} ubench_atomic_int_t;

static inline int
ubench_atomic_int_get(ubench_atomic_int_t* atomic) {
	return atomic->atomic_value;
}

// return old value
static inline int
ubench_atomic_int_inc(ubench_atomic_int_t* atomic) {
#if defined(_MSC_VER)
	return InterlockedIncrement(&atomic->atomic_value) - 1;
#elif defined(__GNUC__)
	return __sync_fetch_and_add(&atomic->atomic_value, 1);
#else
#error "Atomic operations not supported on this platform/compiler."
	atomic->atomic_value++;
#endif
}

static inline int
ubench_atomic_int_reset(ubench_atomic_int_t* atomic) {
#if defined(_MSC_VER)
	return InterlockedAnd(&atomic->atomic_value, 0);
#elif defined(__GNUC__)
	return __sync_fetch_and_and(&atomic->atomic_value, 0);
#else
#error "Atomic operations not supported on this platform/compiler."
	int result = atomic->atomic_value;
	atomic->atomic_value = 0;
	return result;
#endif
}

#endif
