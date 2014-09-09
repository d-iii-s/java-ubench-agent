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

#pragma warning(push, 0)
#include <jvmti.h>
#include <jni.h>
#include <jvmticmlr.h>
#pragma warning(pop)

#ifdef _MSC_VER
/* MSVC offers inline only for C++ code. */
#define inline __inline
#endif

/*
 * FIXME: properly implement
 */

typedef struct ubench_atomic {
	int atomic_value;
} ubench_atomic_t;

static inline void ubench_atomic_init(ubench_atomic_t *atomic, int new_value) {
	atomic->atomic_value = new_value;
}

static inline int ubench_atomic_get(ubench_atomic_t *atomic) {
	return atomic->atomic_value;
}

static inline void ubench_atomic_add(ubench_atomic_t *atomic, int how_much_to_add) {
	atomic->atomic_value += how_much_to_add;
}

static inline int ubench_atomic_get_and_set(ubench_atomic_t *atomic, int new_value) {
	int result = atomic->atomic_value;
	atomic->atomic_value = new_value;
	return result;
}
