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

import java.util.Arrays;

public class BenchmarkTest {
	private static final int LOOPS = 10;
	
	private static final BenchmarkMetric[] columns = {
		BenchmarkMetric.WALL_CLOCK_TIME,
		BenchmarkMetric.CONTEXT_SWITCH_FORCED,
		BenchmarkMetric.L2_DATA_READ
	};
	
	public static void main(String[] args) {
		Benchmark.init(LOOPS, Arrays.asList(columns));		
		
		for (int i = 0; i < LOOPS; i++) {
			long before = System.nanoTime();
			Benchmark.start();
			long start = System.nanoTime();
			System.out.printf("This is loop %d", i);
			long end = System.nanoTime();
			Benchmark.stop();
			long after = System.nanoTime();
			long duration = end - start;
			long start_measurement_duration = start - before;
			long end_measurement_duration = after - end;
			System.out.printf(" [took %dus, measurement %dns and %dns].\n",
					duration / 1000, start_measurement_duration,
					end_measurement_duration);
		}
		
		Benchmark.dump("-");
		
		System.exit(0);
	}
}
