/*
 * Copyright 2015 Charles University in Prague
 * Copyright 2015 Vojtech Horky
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

import org.junit.*;

public class ThreadTimeTest {
	// The rest of the program assumes this ordering!
	private static final String[] EVENTS = {
		"SYS:wallclock-time",
		"SYS:thread-time",
		"SYS:thread-time-rusage"
	};
	private static final String[] EVENTS_NO_RSUAGE = {
		"SYS:wallclock-time",
		"SYS:thread-time"
	};
	
	private static final int TEST_LENGTH_SEC = 8;
	private static final int DELTA_PERCENT = 10;
	private static final int DELTA_MILLIS = TEST_LENGTH_SEC * 1000 * DELTA_PERCENT / 100;
	
	
	
	private static void busyWait(int lengthSec) {
		long endTime = System.currentTimeMillis() + lengthSec * 1000;
		while (System.currentTimeMillis() < endTime) {
			// Busy wait here.
		}
	}
	
	private static void semiBusyWait(int lengthSec) {
		long endTime = System.currentTimeMillis() + lengthSec * 1000;
		// Wait half of the time sleeping.
		TestUtils.noThrowSleep(500 * lengthSec);
		while (System.currentTimeMillis() < endTime) {
			// Busy wait here.
		}
	}
	
	private static void initBenchmark(int count) {
		try {
			Benchmark.init(count, EVENTS);
		} catch (MeasurementException e) {
			Benchmark.init(count, EVENTS_NO_RSUAGE);
		}
	}
	
	private static double[] nanosToMillis(long[] data) {
		double[] result = new double[data.length];
		for (int i = 0; i < data.length; i++) {
			result[i] = data[i] / 1000. / 1000.;
			// System.out.printf("[%d]   %d => %.2f\n", i, data[i], result[i]);
		}
		return result;
	}
	
	@Test
	public void threadTimeOfBusyWait() {
		initBenchmark(1);
		
		Benchmark.start();
		busyWait(TEST_LENGTH_SEC);
		Benchmark.stop();
		
		double[] res = nanosToMillis(Benchmark.getResults().getData().get(0));
				
		Assert.assertEquals(TEST_LENGTH_SEC * 1000, res[0], DELTA_MILLIS);
		Assert.assertEquals(TEST_LENGTH_SEC * 1000, res[1], DELTA_MILLIS);
		if (res.length == 3) {	
			Assert.assertEquals(TEST_LENGTH_SEC * 1000, res[2], DELTA_MILLIS);
		}
	}
	
	@Test
	public void threadTimeOfSemiBusyWait() {
		initBenchmark(1);
		
		Benchmark.start();
		semiBusyWait(TEST_LENGTH_SEC);
		Benchmark.stop();
		
		double[] res = nanosToMillis(Benchmark.getResults().getData().get(0));
		
		Assert.assertEquals(TEST_LENGTH_SEC * 1000, res[0], DELTA_MILLIS);
		Assert.assertEquals(TEST_LENGTH_SEC * 500, res[1], DELTA_MILLIS);
		if (res.length == 3) {	
			Assert.assertEquals(TEST_LENGTH_SEC * 500, res[2], DELTA_MILLIS);
		}
	}

	// For debugging purposes, it is possible to run this directly.
	public static void main(String[] args) {
		int lengthSec = TEST_LENGTH_SEC;
		
		if (args.length == 1) {
			lengthSec = Integer.parseInt(args[0]);
		}
		
		initBenchmark(2);
		
		Benchmark.start();
		busyWait(lengthSec);
		Benchmark.stop();
		
		Benchmark.start();
		semiBusyWait(lengthSec);
		Benchmark.stop();
		
		BenchmarkResults results = Benchmark.getResults();
		System.out.println("        Wall-clock       Thread-time   Via getrusage()");
		for (long[] row : results.getData()) {
			for (long r : row) {
				System.out.printf("%18.2f", r / 1000. / 1000.);
			}
			System.out.println();
		}
	}
}
