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

#include "cz_cuni_mff_d3s_perf_CompilationCounter.h"
#include "microbench.h"

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_init(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass),
		jint measurements) {
	printf("Benchmark.init(%d)\n", measurements);
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_start(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	printf("Benchmark.start()\n");
	fflush(stdout);
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_stop(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	printf("Benchmark.stop()\n");
	fflush(stdout);
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_dump(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass), jstring jfilename) {
	const char *filename = (*env)->GetStringUTFChars(env, jfilename, 0);

	printf("Benchmark.dump(%s)\n", filename);

	(*env)->ReleaseStringUTFChars(env, jfilename, filename);
}

