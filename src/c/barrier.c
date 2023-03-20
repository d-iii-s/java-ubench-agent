/*
 * Copyright 2017 Charles University in Prague
 * Copyright 2017 Vojtech Horky
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
#include "cz_cuni_mff_d3s_perf_Barrier.h"

#ifdef __unix__
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include <jni.h>
#include <jvmti.h>
#pragma warning(pop)

#ifdef __unix__
static int shared_mem_id = -1;
static pthread_barrier_t* shared_mem_barrier = NULL;
#endif

JNIEXPORT void JNICALL
Java_cz_cuni_mff_d3s_perf_Barrier_initNative(
	JNIEnv* jni, jclass UNUSED_PARAMETER(barrier_class), jstring jname
) {
#ifdef __unix__
	const char* name = (*jni)->GetStringUTFChars(jni, jname, 0);
	shared_mem_id = shm_open(name, O_RDWR, 0600);
	(*jni)->ReleaseStringUTFChars(jni, jname, name);
	if (shared_mem_id == -1) {
		// TODO: throw Java exception
		fprintf(stderr, "Failed to create shared memory segment!\n");
		return;
	}

	shared_mem_barrier = (pthread_barrier_t*) mmap(0, sizeof(pthread_barrier_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_id, 0);
	if (shared_mem_barrier == MAP_FAILED) {
		// TODO: throw Java exception
		fprintf(stderr, "Failed to mmap shared memory segment!\n");
		return;
	}
#else
	// Silence the compiler (unused variables).
	UNUSED_VARIABLE(jni);
	UNUSED_VARIABLE(jname);

	fprintf(stderr, "Platform not yet supported.\n");
	return;
	// TODO: throw Java exception about unsupported platform
#endif
}

JNIEXPORT void JNICALL
Java_cz_cuni_mff_d3s_perf_Barrier_barrier(
	JNIEnv* UNUSED_PARAMETER(jni), jclass UNUSED_PARAMETER(barrier_class)
) {
#ifdef __unix__
	pthread_barrier_wait(shared_mem_barrier);
#endif
}
