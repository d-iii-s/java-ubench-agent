/*
 * Copyright 2014-2016 Charles University in Prague
 * Copyright 2014-2016 Vojtech Horky
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

#define _BSD_SOURCE // For glibc 2.18 to include caddr_t
#define _DEFAULT_SOURCE // For glibc 2.20 to include caddr_t
#define _POSIX_C_SOURCE 200809L

#include "ubench.h"

#pragma warning(push, 0)
#include "cz_cuni_mff_d3s_perf_CompilationCounter.h"
#include "cz_cuni_mff_d3s_perf_Benchmark.h"
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#pragma warning(pop)

#ifdef HAS_PAPI
/*
 * Include <sys/types.h> to have caddr_t.
 * Not needed with GCC and -std=c99.
 */
#include <sys/types.h>
#include <papi.h>
#include <pthread.h>
#endif

#ifdef HAS_QUERY_PERFORMANCE_COUNTER
#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)
#endif

static benchmark_configuration_t current_benchmark;

jint ubench_benchmark_init(void) {
#ifdef HAS_PAPI
	// TODO: check for errors
	PAPI_library_init(PAPI_VER_CURRENT);
	int papi_rc = PAPI_thread_init(pthread_self);
	assert(papi_rc == PAPI_OK);
#endif

	current_benchmark.used_backends = 0;
	while (current_benchmark.used_events_count > 0) {
		current_benchmark.used_events_count--;
		free(current_benchmark.used_events[current_benchmark.used_events_count].name);
	}
	current_benchmark.used_events = NULL;
	current_benchmark.used_events_count = 0;

#ifdef HAS_PAPI
	current_benchmark.used_papi_events_count = 0;
	current_benchmark.papi_eventset = PAPI_NULL;
#endif

	current_benchmark.data = NULL;
	current_benchmark.data_index = 0;
	current_benchmark.data_size = 0;

	ubench_event_init();

	return JNI_OK;
}


void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_init(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass),
		jint jmeasurements, jobjectArray jeventNames, jintArray joptions) {
	free(current_benchmark.data);
	free(current_benchmark.used_events);
	current_benchmark.data = NULL;
	current_benchmark.data_index = 0;
	current_benchmark.data_size = 0;

	current_benchmark.used_backends = 0;
	current_benchmark.used_events_count = 0;


#ifdef HAS_PAPI
	current_benchmark.used_papi_events_count = 0;
	PAPI_cleanup_eventset(current_benchmark.papi_eventset);
	PAPI_destroy_eventset(&current_benchmark.papi_eventset);
	current_benchmark.papi_eventset = PAPI_NULL;
#endif

	size_t events_count = (*env)->GetArrayLength(env, jeventNames);
	if (events_count == 0) {
		return;
	}

	if (jmeasurements <= 0) {
		return;
	}

	size_t options_count = (*env)->GetArrayLength(env, joptions);
	jint *options = (*env)->GetIntArrayElements(env, joptions, NULL);

	current_benchmark.data = calloc(jmeasurements, sizeof(benchmark_run_t));
	current_benchmark.data_size = jmeasurements;

	current_benchmark.used_events = calloc(events_count, sizeof(ubench_event_info_t));

	size_t i;
	for (i = 0; i < events_count; i++) {
		jstring jevent_name = (jstring) (*env)->GetObjectArrayElement(env, jeventNames, (jsize) i);
		const char *event_name = (*env)->GetStringUTFChars(env, jevent_name, 0);

		ubench_event_info_t *event_info = &current_benchmark.used_events[current_benchmark.used_events_count];

		int event_ok = ubench_event_resolve(event_name, event_info);
		if (!event_ok) {
			fprintf(stderr, "Unrecognized event %s\n", event_name);
			goto event_loop_end;
		}

		current_benchmark.used_events_count++;

		current_benchmark.used_backends |= event_info->backend;

#ifdef HAS_PAPI
		if (event_info->backend == UBENCH_EVENT_BACKEND_PAPI) {
			/* Check that the id is not already there. */
			int already_registered = 0;
			size_t j;
			for (j = 0; j < current_benchmark.used_papi_events_count; j++) {
				if (current_benchmark.used_papi_events[j] == event_info->id) {
					already_registered = 1;
					event_info->papi_index = j;
					break;
				}
			}
			if (!already_registered) {
				if (current_benchmark.used_papi_events_count < UBENCH_MAX_PAPI_EVENTS) {
					event_info->papi_index = current_benchmark.used_papi_events_count;
					current_benchmark.used_papi_events[current_benchmark.used_papi_events_count] = event_info->id;
					current_benchmark.used_papi_events_count++;
				}
				// FIXME: inform user that there are way to many PAPI events
			}
		}
#endif

event_loop_end:
		(*env)->ReleaseStringUTFChars(env, jevent_name, event_name);
	}

#ifdef HAS_PAPI
	if ((current_benchmark.used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		// FIXME: handle errors
		int rc = PAPI_create_eventset(&current_benchmark.papi_eventset);
		assert(rc == PAPI_OK);

		// TODO: find out why setting the component and inherit flag
		// *before* adding the individual events work
		rc = PAPI_assign_eventset_component(current_benchmark.papi_eventset, 0);
		assert(rc == PAPI_OK);

		for (i = 0; i < options_count; i++) {
			if (options[i] == cz_cuni_mff_d3s_perf_Benchmark_THREAD_INHERIT) {
				PAPI_option_t opt;
				memset(&opt, 0, sizeof(opt));
				opt.inherit.inherit = PAPI_INHERIT_ALL;
				opt.inherit.eventset = current_benchmark.papi_eventset;
				rc = PAPI_set_opt(PAPI_INHERIT, &opt);
				assert(rc == PAPI_OK);
			}
		}

		for (i = 0; i < current_benchmark.used_papi_events_count; i++) {
			rc = PAPI_add_event(current_benchmark.papi_eventset,
					current_benchmark.used_papi_events[i]);
			assert(rc == PAPI_OK);
		}

	}
#endif


#if 0
	for (i = 0; i < current_benchmark.used_events_count; i++) {
		int event_id = current_benchmark.used_events[i];
		fprintf(stderr, "Event #%2zu: %s\n", i, all_known_events_info[event_id].id);
	}
#endif

	(*env)->ReleaseIntArrayElements(env, joptions, options, JNI_ABORT);
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_start(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	if (current_benchmark.data_size == 0) {
		return;
	}
	if (current_benchmark.data_index >= current_benchmark.data_size) {
		current_benchmark.data_index--;
	}

	ubench_events_snapshot_t *snapshot = &current_benchmark.data[current_benchmark.data_index].start;

	ubench_measure_start(&current_benchmark, snapshot);
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_stop(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	ubench_events_snapshot_t *snapshot = &current_benchmark.data[current_benchmark.data_index].end;

	ubench_measure_stop(&current_benchmark, snapshot);

	current_benchmark.data_index++;
}

void JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_reset(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass)) {
	current_benchmark.data_index = 0;
}

jobject JNICALL Java_cz_cuni_mff_d3s_perf_Benchmark_getResults(JNIEnv *env,
		jclass UNUSED_PARAMETER(klass)) {
	jmethodID constructor;
	jclass results_class = (*env)->FindClass(env, "cz/cuni/mff/d3s/perf/BenchmarkResultsImpl");
	if (results_class == NULL) {
		return NULL;
	}
	jclass string_class = (*env)->FindClass(env, "java/lang/String");
	if (string_class == NULL) {
		return NULL;
	}

	jobjectArray jevent_names = (jobjectArray) (*env)->NewObjectArray(env,
		(jsize) current_benchmark.used_events_count,
		string_class, NULL);
	size_t i;
	for (i = 0; i < current_benchmark.used_events_count; i++) {
		(*env)->SetObjectArrayElement(env, jevent_names, (jsize) i, (*env)->NewStringUTF(env, current_benchmark.used_events[i].name));
	}


	constructor = (*env)->GetMethodID(env, results_class, "<init>", "([Ljava/lang/String;)V");
	if (constructor == NULL) {
		return NULL;
	}

	jobject jresults = (*env)->NewObject(env, results_class, constructor, jevent_names);
	if (jresults == NULL) {
		return NULL;
	}

	size_t bi;
	jlongArray event_values = (*env)->NewLongArray(env, (jsize) current_benchmark.used_events_count);
	if (event_values == NULL) {
		return NULL;
	}
	jmethodID set_data_method = (*env)->GetMethodID(env, results_class, "addDataRow", "([J)V");
	if (set_data_method == NULL) {
		return NULL;
	}
	for (bi = 0; bi < current_benchmark.data_index; bi++) {
		benchmark_run_t *benchmark = &current_benchmark.data[bi];

		size_t ei;
		for (ei = 0; ei < current_benchmark.used_events_count; ei++) {
			ubench_event_info_t *event = &current_benchmark.used_events[ei];
			long long value = event->op_get(benchmark, event);
			jlong jvalue = (jlong) value;
			// FIXME: report PAPI errors etc.
			(*env)->SetLongArrayRegion(env, event_values, (jsize) ei, 1, &jvalue);
		}

		(*env)->CallVoidMethod(env, jresults, set_data_method, event_values);
	}

	return jresults;
}
