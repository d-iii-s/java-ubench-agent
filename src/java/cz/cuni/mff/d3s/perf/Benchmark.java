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

package cz.cuni.mff.d3s.perf;

import java.util.ArrayList;
import java.util.List;

public final class Benchmark {
	public static final int THREAD_INHERIT = Measurement.THREAD_INHERIT;
	
	private static int defaultEventSet = -1;
	
	public static void init(final int measurements, final String[] events, int... options) {
		if (defaultEventSet != -1) {
			Measurement.destroyEventSet(defaultEventSet);
			defaultEventSet = -1;
		}
		defaultEventSet = Measurement.createEventSet(measurements, events, options);
	}
	
	public static void start() {
		Measurement.start(defaultEventSet);		
	}
	
	public static void stop() {
		Measurement.stop(defaultEventSet);
	}
	public static void reset() {
		Measurement.reset(defaultEventSet);
	}
	
	public static BenchmarkResults getResults() {
		return Measurement.getResults(defaultEventSet);
	}
	
	public static List<String> getSupportedEvents() {
		List<String> result = Measurement.getSupportedEvents();
		if (result == null) {
			return new ArrayList<>(0);
		}
		return result;
	}
}
