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

package cz.cuni.mff.d3s.perf;

import java.util.ArrayList;
import java.util.List;

/** Java class interfacing with native calls to the actual implementation. */
public final class Measurement {

    /** Use events set for newly created threads too.
     *
     * <p>
     * This is a flag for <code>create*EventSet*</code> calls.
     */
    public static final int THREAD_INHERIT = 1;

    /** Generics' helper. */
    private static final String[] STRING_ARRAY_TYPE = new String[0];
    
    /** Prevent instantiation. */
    private Measurement() {}
    
    /** Create new event set.
     *
     * @param measurementCount How many measurements should remember the C agent.
     * @param events List of events.
     * @param options Extra options (flags).
     * @return Event set number (opaque identifier).
     * @throws cz.cuni.mff.d3s.perf.MeasurementException On any failure when setting-up
     *     the new event set.
     */
    public static native int createEventSet(int measurementCount, String[] events, int... options);

    /** Create new event set attached to a Java thread.
     *
     * @param threadId Jave thread id.
     * @param measurementCount How many measurements should remember the C agent.
     * @param events List of events.
     * @param options Extra options (flags).
     * @return Event set number (opaque identifier).
     * @throws cz.cuni.mff.d3s.perf.MeasurementException On any failure when setting-up
     *     the new event set.
     */
    private static native int createAttachedEventSetWithJavaThread(long threadId,
            int measurementCount, String[] events, int... options);
    
    /** Create new event set attached to a native thread.
     *
     * @param threadId Native thread id (OS-dependent).
     * @param measurementCount How many measurements should remember the C agent.
     * @param events List of events.
     * @param options Extra options (flags).
     * @return Event set number (opaque identifier).
     * @throws cz.cuni.mff.d3s.perf.MeasurementException On any failure when setting-up
     *     the new event set.
     */
    private static native int createAttachedEventSetWithNativeThread(long threadId,
            int measurementCount, String[] events, int... options);

    /** Create new event set attached to a Java thread.
     *
     * @param thread Jave thread
     * @param measurementCount How many measurements should remember the C agent.
     * @param events List of events.
     * @param options Extra options (flags).
     * @return Event set number (opaque identifier).
     * @throws cz.cuni.mff.d3s.perf.MeasurementException On any failure when setting-up
     *     the new event set.
     */
    public static int createAttachedEventSet(final Thread thread, final int measurementCount,
            final String[] events, final int... options) {
        return createAttachedEventSetWithJavaThread(thread.getId(), measurementCount,
                events, options);
    }

    /** Create new event set attached to a native thread.
     *
     * @param thread Native thread id (OS-dependent).
     * @param measurementCount How many measurements should remember the C agent.
     * @param events List of events.
     * @param options Extra options (flags).
     * @return Event set number (opaque identifier).
     * @throws cz.cuni.mff.d3s.perf.MeasurementException On any failure when setting-up
     *     the new event set.
     */
    public static int createAttachedEventSetOnNativeThread(final long thread,
            final int measurementCount, final String[] events, final int... options) {
        return createAttachedEventSetWithNativeThread(thread, measurementCount, events, options);
    }

    /** Destroy existing event set.
     *
     * @param eventSet Event set to destroy.
     * @throws cz.cuni.mff.d3s.perf.MeasurementException Invalid event set identification.
     */
    public static native void destroyEventSet(int eventSet);

    /** Start actual measurement.
     *
     * @param eventSet Array of event sets where the measurements are started.
     */
    public static native void start(int... eventSet);
    
    /** Stop actual measurement.
     *
     * @param eventSet Array of event sets where the measurements are stopped.
     */
    public static native void stop(int... eventSet);
    
    /** Sample the event counters.
     *
     * <p>
     * Useful when you do not want to run stop-start in the middle of the
     * execution to get counters state.
     *
     * <p>
     * Use getRawResults() to obtain results including the sampling.
     *
     * @param sampleId User id to distinguish different calls.
     * @param eventSet Array of event sets where the measurements should be sampled.
     */
    public static native void sample(int sampleId, int... eventSet);

    /** Clear measurements.
     *
     * @param eventSet Array of event sets where the measurements are removed.
     */
    public static native void reset(int... eventSet);

    /** Retrieve results for one event set.
     *
     * @param eventSet Event set identification.
     * @return Measurement results.
     */
    public static native BenchmarkResults getResults(int eventSet);

    /** Retrieve all results for one event set.
     *
     * <p>
     * Note that the last column would always be TYPE with number -1 to
     * denote call to start(), -2 call to stop() and positive numbers
     * denoting parameters from sample() call.
     *
     * @param eventSet Event set identification.
     * @return Measurement results.
     */
    public static native BenchmarkResults getRawResults(int eventSet);

    /** Checks that event is supported.
     *
     * @param event Event name.
     * @return Whether event is supported on current platform.
     */
    public static native boolean isEventSupported(String event);

    /** Filter list of events to contain only events supported on current platform.
     *
     * @param events List of event names.
     * @return List of events supported on current platform.
     */
    public static String[] filterSupportedEvents(final String[] events) {
        List<String> result = new ArrayList<>(events.length);
        for (String ev : events) {
            if (Measurement.isEventSupported(ev)) {
                result.add(ev);
            }
        }
        return result.toArray(STRING_ARRAY_TYPE);
    }

    /** Get list of events supported on current platform.
     *
     * <p>
     * This method may return NULL in case of internal error.
     *
     * @return List of supported events.
     */
    public static native List<String> getSupportedEvents();
}
