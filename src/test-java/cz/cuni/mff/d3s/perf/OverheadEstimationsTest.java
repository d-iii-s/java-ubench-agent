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
package cz.cuni.mff.d3s.perf;

import java.util.*;

import org.junit.*;

@Ignore
public class OverheadEstimationsTest {
	private static final int LOOPS = 10;
	private static final int INNER_LOOPS = 1000000;
	
	public static void main(String[] args) {
		Benchmark.init(LOOPS * 2, new String[] { "clock-monotonic" });
		
		for (int loop = 0; loop < LOOPS; loop++) {
			Benchmark.start();
			for (int i = 0; i < INNER_LOOPS; i++) {
				OverheadEstimations.emptyNativeCall();
			}
			Benchmark.stop();
		
			Benchmark.start();
			for (int i = 0; i < INNER_LOOPS; i++) {
				OverheadEstimations.resourceUsageCall();
			}
			Benchmark.stop();
		}
		
		ArrayList<long[]> data = new ArrayList<>();
		for (long[] row : Benchmark.getResults().getData()) {
			data.add(row);
		}
		
		if (data.size() != LOOPS * 2) {
			System.err.printf("Expected %d values but got %d.\n", LOOPS * 2, data.size());
			System.exit(1);
		}
		
		long sumEmptyNativeCalls = 0;
		long sumResourceUsageCalls = 0;
		for (int i = 0; i < data.size(); i += 2) {
			sumEmptyNativeCalls += data.get(i)[0];
			sumResourceUsageCalls += data.get(i + 1)[0];
		}
		
		double avgEmptyNativeCall = (double) sumEmptyNativeCalls / LOOPS / INNER_LOOPS;
		double avgResourceUsageCall = (double) sumResourceUsageCalls / LOOPS / INNER_LOOPS;
		System.out.printf("Average for empty native call is %.2fns, getrusage() takes about %.2fns (%.0f%%).\n",
			avgEmptyNativeCall, avgResourceUsageCall,
			100 * avgResourceUsageCall / avgEmptyNativeCall);
		
		System.exit(0);
	}
}
