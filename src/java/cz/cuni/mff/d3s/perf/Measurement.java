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

public final class Measurement {
	public static final int THREAD_INHERIT = 1;
	public static native int createEventSet(final int measurementCount, final String[] events, int ... options);
	
	private static native int createAttachedEventSetNative(final long threadId, final int measurementCount, final String[] events, int ... options);
	
	public static int createAttachedEventSet(final Thread thread, final int measurementCount, final String[] events, int ... options) {
		return createAttachedEventSetNative(thread.getId(), measurementCount, events, options);
	}
	public static native void destroyEventSet(final int eventSet);
	
	public static native void start(final int ... eventSet);
	public static native void stop(final int ... eventSet);
	public static native void reset(final int ... eventSet);
	
	public static native BenchmarkResults getResults(final int eventSet);
	
	public static native boolean isEventSupported(final String event);
	
	private static final String[] STRING_ARRAY_TYPE = new String[0];
	
	public static String[] filterSupportedEvents(final String[] events) {
		List<String> result = new ArrayList<>(events.length);
		for (String ev : events) {
			if (Measurement.isEventSupported(ev)) {
				result.add(ev);
			}
		}
		return result.toArray(STRING_ARRAY_TYPE);
	}
}
