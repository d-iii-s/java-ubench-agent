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


public class BenchmarkTest {
	private static final String[] SMOKE_TEST_EVENTS = { "SYS:wallclock-time" };
	private static final int SMOKE_TEST_SLEEP_MILLIS = 100;
	private static final int LOOPS = 5;
	
	@Test
	public void getResultsSmokeTest() throws InterruptedException {
		Benchmark.init(1, SMOKE_TEST_EVENTS);
		Benchmark.start();
		Thread.sleep(SMOKE_TEST_SLEEP_MILLIS);
		Benchmark.stop();
		
		
		BenchmarkResults results = Benchmark.getResults();
		Assert.assertNotNull("getResults() may not return null", results);
		
		String[] eventNames = results.getEventNames();
		Assert.assertNotNull("getEventNames() may not return null", eventNames);
		Assert.assertArrayEquals(SMOKE_TEST_EVENTS, eventNames);
		
		List<long[]> data = results.getData();
		Assert.assertNotNull("getData() may not return null", data);
		
		Assert.assertEquals("we collected single sample only", 1, data.size());
		
		long[] numbers = data.get(0);
		Assert.assertNotNull("the array with results ought not to be null", numbers);
		Assert.assertEquals("we collected single event only", 1, numbers.length);
		
		Assert.assertTrue("sample " + numbers[0] + " is not in range",
			(numbers[0] > SMOKE_TEST_SLEEP_MILLIS*1000*1000/2)
			&& (numbers[0] < SMOKE_TEST_SLEEP_MILLIS*1000*1000*10));
	}
	
	@Test
	public void resetWorks() throws InterruptedException {
		Benchmark.init(LOOPS, SMOKE_TEST_EVENTS);
		for (int i = 0; i < LOOPS; i++) {
			Benchmark.start();
			Thread.sleep(SMOKE_TEST_SLEEP_MILLIS);
			Benchmark.stop();
		}
		
		Benchmark.reset();
		
		for (int i = 0; i < LOOPS / 2; i++) {
			Benchmark.start();
			Thread.sleep(SMOKE_TEST_SLEEP_MILLIS);
			Benchmark.stop();
		}
		
		BenchmarkResults results = Benchmark.getResults();
		List<long[]> data = results.getData();
		
		Assert.assertEquals(LOOPS / 2, data.size());
	}
	
	private static final String[] EVENTS = {
		"SYS:wallclock-time",
		"SYS:forced-context-switches",
		"JVM:compilations",
		"PAPI_L1_DCM",
		"PAPI_L1_DCM",
	};
	private static final String[] SUPPORTED_EVENTS = Measurement.filterSupportedEvents(EVENTS);
	
	public static void main(String[] args) {
		
		Benchmark.init(LOOPS, SUPPORTED_EVENTS);
		for (int i = 0; i < LOOPS; i++) {
			Benchmark.start();
			Benchmark.stop();
		}
		
		Benchmark.init(LOOPS, SUPPORTED_EVENTS);	
		
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
		
		BenchmarkResultsPrinter.table(Benchmark.getResults(), System.out);
		// BenchmarkResultsPrinter.toCsv(Benchmark.getResults(), System.out, ",", true);

		System.exit(0);
	}
}
