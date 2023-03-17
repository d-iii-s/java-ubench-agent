/*
 * Copyright 2018 Charles University in Prague
 * Copyright 2018 Vojtech Horky
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

#pragma warning(push, 0)
#include <jni.h>
#include <jvmti.h>
#include <jvmticmlr.h>
#pragma warning(pop)

#ifdef _MSC_VER
/* MSVC offers inline only for C++ code. */
#define inline __inline
#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)
#endif

#define UBENCH_SPINLOCK_INITIALIZER { 0 }

typedef struct {
#ifdef _MSC_VER
	LONG value;
#else
	int value;
#endif
} ubench_spinlock_t;

static inline void
ubench_spinlock_lock(ubench_spinlock_t* spinlock) {
#if defined(_MSC_VER)
	while (InterlockedCompareExchange(&spinlock->value, 1, 0) == 1) {}
#elif defined(__GNUC__)
	while (!__sync_bool_compare_and_swap(&spinlock->value, 0, 1)) {}
#else
#error "Spinlock not supported on this platform/compiler."
	while (spinlock->value) {}
	spinlock->value = 1;
#endif
}

static inline void
ubench_spinlock_unlock(ubench_spinlock_t* spinlock) {
#if defined(_MSC_VER)
	InterlockedAnd(&spinlock->value, 0);
#elif defined(__GNUC__)
	__sync_fetch_and_and(&spinlock->value, 0);
#else
#error "Spinlock not supported on this platform/compiler."
	spinlock->value = 0;
#endif
}
