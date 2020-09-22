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

#define _BSD_SOURCE // For glibc 2.18 to include caddr_t
#define _DEFAULT_SOURCE // For glibc 2.20 to include caddr_t
#define _POSIX_C_SOURCE 200809L

#include "ubench.h"

#pragma warning(push, 0)
#include "cz_cuni_mff_d3s_perf_CompilationCounter.h"
#include "cz_cuni_mff_d3s_perf_Measurement.h"
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
// Need the syscall function
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#endif

#ifdef HAS_QUERY_PERFORMANCE_COUNTER
#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)
#endif


typedef struct {
	benchmark_configuration_t config;
	int valid;
} eventset_t;

static eventset_t *all_eventsets = NULL;
/* We use jint as we compare the passed IDs with this value. */
static jint all_eventset_count = 0;


#ifdef HAS_PAPI
static void
__die_with_papi_error (int papi_error, const char * message) {
	fprintf (
		stderr, "error: %s: %s (%d)\n",
		message, PAPI_strerror (papi_error), papi_error
	);

	exit (1);
}

static void
__check_papi_error (int papi_error, const char *message) {
	if (papi_error < 0) {
		__die_with_papi_error (papi_error, message);
	}
}

static unsigned long get_thread_id(void) {
	pid_t answer = syscall (__NR_gettid);
	return (unsigned long) answer;
}
#endif

jint ubench_benchmark_init(void) {
#ifdef HAS_PAPI
	// TODO: check for errors
	int lib_init_rc = PAPI_library_init(PAPI_VER_CURRENT);
	if (lib_init_rc != PAPI_VER_CURRENT && lib_init_rc > 0) {
		fprintf(stderr,"error: PAPI library version mismatch!\n");
		exit(1);
	}

	__check_papi_error (lib_init_rc, "PAPI library initialization");

	int thread_init_rc = PAPI_thread_init(get_thread_id);
	__check_papi_error (thread_init_rc, "PAPI thread initialization");

	//PAPI_set_debug(PAPI_VERB_ECONT);
#endif

	ubench_event_init();

	return JNI_OK;
}

static void do_throw(JNIEnv *env, const char *message) {
	jclass exClass = (*env)->FindClass(env, "cz/cuni/mff/d3s/perf/MeasurementException");
	if (exClass == NULL) {
		fprintf(stderr, "Unable to find MeasurementException class, aborting!\n");
		exit (1);
	}
	(*env)->ThrowNew(env, exClass, message);
}

#ifdef HAS_PAPI
static void do_papi_error_throw(JNIEnv *env, int rc, const char *function_that_failed) {
	char message[512];
	sprintf(message, "%s failed: %s.", function_that_failed, PAPI_strerror(rc));
	do_throw(env, message);
}
#endif

#define THROW_OOM(env, message) \
	do_throw(env, "Out of memory (" message ").")


DLL_EXPORT jint JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_createEventSet(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass),
		jint jmeasurements, jobjectArray jeventNames, jintArray joptions) {
#ifndef HAS_PAPI
	UNUSED_VARIABLE(joptions);
#endif

	// FIXME: where to properly compute this number
	jmeasurements *= 2;

	if (jmeasurements <= 0) {
		do_throw(env, "Number of measurements has to be positive.");
		return -1;
	}

	size_t event_count = (*env)->GetArrayLength(env, jeventNames);
	if (event_count == 0) {
		do_throw(env, "List of events cannot be empty.");
		return - 1;
	}

	// FIXME: locking

	/*
	 * First find whether some id is free, then reallocate.
	 */
	eventset_t *eventset = NULL;
	jint eventset_id = -1;
	for (jint i = 0; i < all_eventset_count; i++) {
		if (!all_eventsets[i].valid) {
			eventset = &all_eventsets[i];
			eventset_id = i;
			break;
		}
	}

	if (eventset == NULL) {
		eventset_t *new_eventset = realloc(all_eventsets, sizeof(eventset_t) * (all_eventset_count + 1));
		if (new_eventset == NULL) {
			THROW_OOM(env, "allocating an event set");
			return -1;
		}
		all_eventsets = new_eventset;

		eventset = &all_eventsets[ all_eventset_count ];
		eventset->valid = 0;
		eventset_id = all_eventset_count;

		all_eventset_count++;
	}

	eventset->config.used_backends = 0;

	eventset->config.data = calloc(jmeasurements, sizeof(ubench_events_snapshot_t));
	if (eventset->config.data == NULL) {
		THROW_OOM(env, "allocating place for measurements");
		return -1;
	}
	eventset->config.data_index = 0;
	eventset->config.data_size = jmeasurements;

	eventset->config.used_events = calloc(event_count, sizeof(ubench_event_info_t));
	if (eventset->config.used_events == NULL) {
		free(eventset->config.data);
		THROW_OOM(env, "allocating place for event metadata");
		return -1;
	}
	eventset->config.used_events_count = 0;

#ifdef HAS_PAPI
	eventset->config.papi_eventset = PAPI_NULL;
	eventset->config.papi_component = 0;
	eventset->config.used_papi_events_count = 0;
#endif

	for (size_t i = 0; i < event_count; i++) {
		jstring jevent_name = (jstring) (*env)->GetObjectArrayElement(env, jeventNames, (jsize) i);
		const char *event_name = (*env)->GetStringUTFChars(env, jevent_name, 0);

		ubench_event_info_t *event_info = &eventset->config.used_events[ eventset->config.used_events_count ];

		int event_ok = ubench_event_resolve(event_name, event_info);
		if (!event_ok) {
			char buf[512];
#ifdef _MSC_VER
			_snprintf_s(buf, 510, _TRUNCATE, "Unrecognized event %s.", event_name);
#else
			snprintf(buf, 510, "Unrecognized event %s.", event_name);
#endif
			buf[511] = 0;
			(*env)->ReleaseStringUTFChars(env, jevent_name, event_name);
			free(eventset->config.used_events);
			free(eventset->config.data);
			do_throw(env, buf);
			return -1;
		}

		eventset->config.used_events_count++;

		eventset->config.used_backends |= event_info->backend;

#ifdef HAS_PAPI
		if (event_info->backend == UBENCH_EVENT_BACKEND_PAPI) {
			/* Check that the id is not already there. */
			int already_registered = 0;
			size_t j;
			for (j = 0; j < eventset->config.used_papi_events_count; j++) {
				if (eventset->config.used_papi_events[j] == event_info->id) {
					already_registered = 1;
					event_info->papi_index = j;
					break;
				}
			}
			if (!already_registered) {
				if (eventset->config.used_papi_events_count < UBENCH_MAX_PAPI_EVENTS) {
					event_info->papi_index = eventset->config.used_papi_events_count;
					eventset->config.used_papi_events[eventset->config.used_papi_events_count] = event_info->id;
					eventset->config.used_papi_events_count++;
				}
				// FIXME: inform user that there are way to many PAPI events
			}

			/* Check that the component is the same for all events. */
			if (eventset->config.used_papi_events_count > 1) {
				if (eventset->config.papi_component != event_info->papi_component) {
					//fprintf(stderr, "so far %d,  new one %d (events %d)\n", eventset->config.papi_component, event_info->papi_component, eventset->config.used_papi_events_count);
					// FIXME: release memory
					do_throw(env, "PAPI components are not the same in the event set.");
					return -1;
				}
			}

			eventset->config.papi_component = event_info->papi_component;
		}
#endif

		(*env)->ReleaseStringUTFChars(env, jevent_name, event_name);
	}

#ifdef HAS_PAPI
	size_t option_count = (*env)->GetArrayLength(env, joptions);
	jint *options = (*env)->GetIntArrayElements(env, joptions, NULL);


	if ((eventset->config.used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		int rc = PAPI_create_eventset(&eventset->config.papi_eventset);
		if (rc != PAPI_OK) {
			(*env)->ReleaseIntArrayElements(env, joptions, options, JNI_ABORT);
			free(eventset->config.used_events);
			free(eventset->config.data);
			do_papi_error_throw(env, rc, "PAPI_create_eventset");
			return -1;
		}

		DEBUG_PRINTF("Created event set %d.", eventset->config.papi_eventset);

		// TODO: find out why setting the component and inherit flag
		// *before* adding the individual events work
		rc = PAPI_assign_eventset_component(eventset->config.papi_eventset, eventset->config.papi_component);
		if (rc != PAPI_OK) {
			(*env)->ReleaseIntArrayElements(env, joptions, options, JNI_ABORT);
			free(eventset->config.used_events);
			free(eventset->config.data);
			do_papi_error_throw(env, rc, "PAPI_assign_eventset_component");
			return -1;
		}

		for (size_t i = 0; i < option_count; i++) {
			if (options[i] == cz_cuni_mff_d3s_perf_Measurement_THREAD_INHERIT) {
				PAPI_option_t opt;
				memset(&opt, 0, sizeof(opt));
				opt.inherit.inherit = PAPI_INHERIT_ALL;
				opt.inherit.eventset = eventset->config.papi_eventset;
				rc = PAPI_set_opt(PAPI_INHERIT, &opt);
				if (rc != PAPI_OK) {
					(*env)->ReleaseIntArrayElements(env, joptions, options, JNI_ABORT);
					free(eventset->config.used_events);
					free(eventset->config.data);
					do_papi_error_throw(env, rc, "PAPI_set_opt(PAPI_INHERIT)");
					return -1;
				}
			}
		}

		for (size_t i = 0; i < eventset->config.used_papi_events_count; i++) {
			rc = PAPI_add_event(eventset->config.papi_eventset,
					eventset->config.used_papi_events[i]);
			if (rc != PAPI_OK) {
				(*env)->ReleaseIntArrayElements(env, joptions, options, JNI_ABORT);
				free(eventset->config.used_events);
				free(eventset->config.data);
				do_papi_error_throw(env, rc, "PAPI_add_event");
				return -1;
			}
		}

	}

	(*env)->ReleaseIntArrayElements(env, joptions, options, JNI_ABORT);

#endif
	eventset->valid = 1;

	return eventset_id;
}

DLL_EXPORT jint JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_createAttachedEventSetWithJavaThread(
		JNIEnv *env, jclass klass,
		jlong jthread_id, jint jmeasurements, jobjectArray jeventNames, jintArray joptions) {
	jint eventset_index = Java_cz_cuni_mff_d3s_perf_Measurement_createEventSet(env, klass, jmeasurements, jeventNames, joptions);

#ifdef HAS_PAPI
	if  ((all_eventsets[ eventset_index ].config.used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		long long native_id = ubench_get_native_thread_id(jthread_id);

		if (native_id == UBENCH_THREAD_ID_INVALID) {
			Java_cz_cuni_mff_d3s_perf_Measurement_destroyEventSet(env, klass, eventset_index);
			do_throw(env, "Unknown thread (not registered with PAPI).");
			return -1;
		}

		DEBUG_PRINTF("Trying to attach %d to %llu (%ld).", eventset_index, native_id, jthread_id);

		int rc = PAPI_attach(all_eventsets[ eventset_index ].config.papi_eventset, (unsigned long) native_id);
		if (rc != PAPI_OK) {
			Java_cz_cuni_mff_d3s_perf_Measurement_destroyEventSet(env, klass, eventset_index);
			do_papi_error_throw(env, rc, "PAPI_attach");
			return -1;
		}
		DEBUG_PRINTF("Attached %d to %llu (%ld).", all_eventsets[ eventset_index ].config.papi_eventset, native_id, jthread_id);
	}
#else
	UNUSED_VARIABLE(joptions);
	UNUSED_VARIABLE(jthread_id);
#endif

	return eventset_index;
}

DLL_EXPORT jint JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_createAttachedEventSetWithNativeThread(
		JNIEnv *env, jclass klass,
		jlong thread_id, jint jmeasurements, jobjectArray jeventNames, jintArray joptions) {
	jint eventset_index = Java_cz_cuni_mff_d3s_perf_Measurement_createEventSet(env, klass, jmeasurements, jeventNames, joptions);

#ifdef HAS_PAPI
	if  ((all_eventsets[ eventset_index ].config.used_backends & UBENCH_EVENT_BACKEND_PAPI) > 0) {
		DEBUG_PRINTF("Trying to attach %d to %lld.", eventset_index, (long long) thread_id);

		int rc = PAPI_attach(all_eventsets[ eventset_index ].config.papi_eventset, (unsigned long) thread_id);
		if (rc != PAPI_OK) {
			Java_cz_cuni_mff_d3s_perf_Measurement_destroyEventSet(env, klass, eventset_index);
			do_papi_error_throw(env, rc, "PAPI_attach");
			return -1;
		}
		DEBUG_PRINTF("Attached %d to %lld.", all_eventsets[ eventset_index ].config.papi_eventset, (long long) thread_id);
	}
#else
	UNUSED_VARIABLE(joptions);
	UNUSED_VARIABLE(thread_id);
#endif

	return eventset_index;
}

DLL_EXPORT void JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_destroyEventSet(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass), jint jid) {
	if ((jid < 0) || (jid >= all_eventset_count) || !all_eventsets[jid].valid) {
		do_throw(env, "Invalid event set id.");
		return;
	}

	free(all_eventsets[jid].config.used_events);
	free(all_eventsets[jid].config.data);
	all_eventsets[jid].valid = 0;
}



DLL_EXPORT void JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_start(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass), jintArray jids) {
	size_t jids_count = (*env)->GetArrayLength(env, jids);
	if (jids_count == 0) {
		return;
	}

	jint *ids = (*env)->GetIntArrayElements(env, jids, NULL);
	for (size_t i = 0; i < jids_count; i++) {
		jint id = ids[i];

		if ((id < 0) || (id >= all_eventset_count) || !all_eventsets[id].valid) {
			do_throw(env, "Invalid event set id.");
			return;
		}

		if (all_eventsets[ id ].config.data_size == 0) {
			continue;
		}

		if (all_eventsets[ id ].config.data_index >= all_eventsets[ id ].config.data_size) {
			all_eventsets[ id ].config.data_index -= 2;
		}

		ubench_events_snapshot_t *snapshot = &all_eventsets[ id ].config.data[ all_eventsets[ id ].config.data_index ];

		ubench_measure_start(&all_eventsets[ id ].config, snapshot);
		all_eventsets[ id ].config.data_index++;
	}

	(*env)->ReleaseIntArrayElements(env, jids, ids, JNI_ABORT);
}

DLL_EXPORT void JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_stop(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass), jintArray jids) {
	size_t jids_count = (*env)->GetArrayLength(env, jids);
	if (jids_count == 0) {
		return;
	}

	jint *ids = (*env)->GetIntArrayElements(env, jids, NULL);
	for (size_t i = 0; i < jids_count; i++) {
		jint id = ids[i];

		if ((id < 0) || (id >= all_eventset_count) || !all_eventsets[id].valid) {
			do_throw(env, "Invalid event set id.");
			return;
		}

		ubench_events_snapshot_t *snapshot = &all_eventsets[ id ].config.data[ all_eventsets[ id ].config.data_index ];

		ubench_measure_stop(&all_eventsets[ id ].config, snapshot);

		all_eventsets[ id ].config.data_index++;
	}

	(*env)->ReleaseIntArrayElements(env, jids, ids, JNI_ABORT);
}

DLL_EXPORT void JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_sample(
		JNIEnv *env, jclass UNUSED_PARAMETER(klass), jint juser_id, jintArray jids) {
	size_t jids_count = (*env)->GetArrayLength(env, jids);
	if (jids_count == 0) {
		return;
	}

	jint *ids = (*env)->GetIntArrayElements(env, jids, NULL);
	for (size_t i = 0; i < jids_count; i++) {
		jint id = ids[i];

		if ((id < 0) || (id >= all_eventset_count) || !all_eventsets[id].valid) {
			do_throw(env, "Invalid event set id.");
			return;
		}

		if (all_eventsets[ id ].config.data_size == 0) {
			continue;
		}

		if (all_eventsets[ id ].config.data_index >= all_eventsets[ id ].config.data_size) {
			all_eventsets[ id ].config.data_index -= 2;
		}

		ubench_events_snapshot_t *snapshot = &all_eventsets[ id ].config.data[ all_eventsets[ id ].config.data_index ];

		ubench_measure_sample(&all_eventsets[ id ].config, snapshot, (int)juser_id);
		all_eventsets[ id ].config.data_index++;
	}

	(*env)->ReleaseIntArrayElements(env, jids, ids, JNI_ABORT);
}

DLL_EXPORT void JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_reset(
		JNIEnv *UNUSED_PARAMETER(env), jclass UNUSED_PARAMETER(klass), jintArray jids) {
	size_t jids_count = (*env)->GetArrayLength(env, jids);
	if (jids_count == 0) {
		return;
	}

	jint *ids = (*env)->GetIntArrayElements(env, jids, NULL);
	for (size_t i = 0; i < jids_count; i++) {
		jint id = ids[i];

		if ((id < 0) || (id >= all_eventset_count) || !all_eventsets[id].valid) {
			do_throw(env, "Invalid event set id.");
			return;
		}

		all_eventsets[ id ].config.data_index = 0;
	}

	(*env)->ReleaseIntArrayElements(env, jids, ids, JNI_ABORT);
}

static size_t find_first_matching_snapshot_type(ubench_events_snapshot_t *snapshots,
		size_t start_index, size_t max_index, int type) {
	if (start_index == (size_t)-1) {
		return (size_t)-1;
	}
	size_t i = start_index;
	while (i < max_index) {
		if (snapshots[i].type == type) {
			return i;
		}
		i++;
	}
	return (size_t)-1;
}

DLL_EXPORT jobject JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_getResults(JNIEnv *env,
		jclass UNUSED_PARAMETER(klass), jint jid) {
	if ((jid < 0) || (jid >= all_eventset_count) || !all_eventsets[jid].valid) {
		do_throw(env, "Invalid event set id.");
		return NULL;
	}

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
		(jsize) all_eventsets[ jid ].config.used_events_count,
		string_class, NULL);
	size_t i;
	for (i = 0; i < all_eventsets[ jid ].config.used_events_count; i++) {
		(*env)->SetObjectArrayElement(env, jevent_names, (jsize) i, (*env)->NewStringUTF(env, all_eventsets[ jid ].config.used_events[i].name));
	}


	constructor = (*env)->GetMethodID(env, results_class, "<init>", "([Ljava/lang/String;)V");
	if (constructor == NULL) {
		return NULL;
	}

	jobject jresults = (*env)->NewObject(env, results_class, constructor, jevent_names);
	if (jresults == NULL) {
		return NULL;
	}

	jlongArray event_values = (*env)->NewLongArray(env, (jsize) all_eventsets[ jid ].config.used_events_count);
	if (event_values == NULL) {
		return NULL;
	}
	jmethodID add_data_method = (*env)->GetMethodID(env, results_class, "addDataRow", "([J)V");
	if (add_data_method == NULL) {
		return NULL;
	}

	i = 0;
	size_t i_max = all_eventsets[ jid ].config.data_index;
	while (i < i_max) {
		ubench_events_snapshot_t *snapshots = all_eventsets[jid].config.data;
		size_t start_index = find_first_matching_snapshot_type(snapshots, i, i_max, UBENCH_SNAPSHOT_TYPE_START);
		size_t end_index = find_first_matching_snapshot_type(snapshots, start_index, i_max, UBENCH_SNAPSHOT_TYPE_END);

		if (end_index == (size_t) -1) {
			break;
		}

		i = end_index + 1;

		size_t ei;
		for (ei = 0; ei < all_eventsets[ jid ].config.used_events_count; ei++) {
			ubench_event_info_t *event = &all_eventsets[ jid ].config.used_events[ei];
			long long value = event->op_get(&snapshots[start_index], &snapshots[end_index], event);
			jlong jvalue = (jlong) value;
			// FIXME: report PAPI errors etc.
			(*env)->SetLongArrayRegion(env, event_values, (jsize) ei, 1, &jvalue);
		}

		(*env)->CallVoidMethod(env, jresults, add_data_method, event_values);
	}

	return jresults;
}


DLL_EXPORT jobject JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_getRawResults(JNIEnv *env,
		jclass UNUSED_PARAMETER(klass), jint jid) {
	if ((jid < 0) || (jid >= all_eventset_count) || !all_eventsets[jid].valid) {
		do_throw(env, "Invalid event set id.");
		return NULL;
	}

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
		(jsize) all_eventsets[ jid ].config.used_events_count + 1,
		string_class, NULL);
	size_t i;
	for (i = 0; i < all_eventsets[ jid ].config.used_events_count; i++) {
		(*env)->SetObjectArrayElement(env, jevent_names, (jsize) i, (*env)->NewStringUTF(env, all_eventsets[ jid ].config.used_events[i].name));
	}
	(*env)->SetObjectArrayElement(env, jevent_names, (jsize) all_eventsets[ jid ].config.used_events_count, (*env)->NewStringUTF(env, "TYPE"));


	constructor = (*env)->GetMethodID(env, results_class, "<init>", "([Ljava/lang/String;)V");
	if (constructor == NULL) {
		return NULL;
	}

	jobject jresults = (*env)->NewObject(env, results_class, constructor, jevent_names);
	if (jresults == NULL) {
		return NULL;
	}

	jlongArray event_values = (*env)->NewLongArray(env, (jsize) all_eventsets[ jid ].config.used_events_count + 1);
	if (event_values == NULL) {
		return NULL;
	}
	jmethodID add_data_method = (*env)->GetMethodID(env, results_class, "addDataRow", "([J)V");
	if (add_data_method == NULL) {
		return NULL;
	}

	i = 0;
	size_t i_max = all_eventsets[ jid ].config.data_index;
	while (i < i_max) {
		size_t ei;
		for (ei = 0; ei < all_eventsets[ jid ].config.used_events_count; ei++) {
			ubench_event_info_t *event = &all_eventsets[ jid ].config.used_events[ei];
			long long value = event->op_get_raw(&all_eventsets[jid].config.data[i], event);
			jlong jvalue = (jlong) value;
			// FIXME: report PAPI errors etc.
			(*env)->SetLongArrayRegion(env, event_values, (jsize) ei, 1, &jvalue);
		}
		jlong type = (long) all_eventsets[jid].config.data[i].type;
		(*env)->SetLongArrayRegion(env, event_values, (jsize) all_eventsets[ jid ].config.used_events_count, 1, &type);

		(*env)->CallVoidMethod(env, jresults, add_data_method, event_values);

		i++;
	}

	return jresults;
}


DLL_EXPORT jboolean JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_isEventSupported(JNIEnv *env,
		jclass UNUSED_PARAMETER(klass), jstring jevent) {
	const char *event = (*env)->GetStringUTFChars(env, jevent, 0);

	ubench_event_info_t info;
	info.name = NULL;

	int result = ubench_event_resolve(event, &info);

	if (info.name != NULL) {
		free(info.name);
	}
	(*env)->ReleaseStringUTFChars(env, jevent, event);

	return result ? JNI_TRUE : JNI_FALSE;
}

struct adding_supported_events_data {
	JNIEnv *env;
	jobject event_list;
	jmethodID add_method;
};

static int adding_supported_events_callback(const char *name, void *arg) {
	struct adding_supported_events_data *j = arg;

	jstring jname = (*j->env)->NewStringUTF(j->env, name);

	(*j->env)->CallBooleanMethod(j->env, j->event_list, j->add_method, jname);

	return 0;
}

DLL_EXPORT jobject JNICALL Java_cz_cuni_mff_d3s_perf_Measurement_getSupportedEvents(JNIEnv *env,
		jclass UNUSED_PARAMETER(klass)) {
	jclass array_list_class = (*env)->FindClass(env, "java/util/ArrayList");
	if (array_list_class == NULL) {
		return NULL;
	}
	jclass string_class = (*env)->FindClass(env, "java/lang/String");
	if (string_class == NULL) {
		return NULL;
	}
	jmethodID constructor = (*env)->GetMethodID(env, array_list_class, "<init>", "()V");
	if (constructor == NULL) {
		return NULL;
	}
	jmethodID add_method = (*env)->GetMethodID(env, array_list_class, "add", "(Ljava/lang/Object;)Z");
	if (add_method == NULL) {
		return NULL;
	}

	jobject jresults = (*env)->NewObject(env, array_list_class, constructor);
	if (jresults == NULL) {
		return NULL;
	}

	struct adding_supported_events_data callback_data;
	callback_data.env = env;
	callback_data.event_list = jresults;
	callback_data.add_method = add_method;

	ubench_event_iterate(adding_supported_events_callback, &callback_data);

	return jresults;
}

