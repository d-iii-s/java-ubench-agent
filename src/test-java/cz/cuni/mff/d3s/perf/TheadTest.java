/*
 * Copyright 2016 Charles University in Prague
 * Copyright 2016 Vojtech Horky
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

import java.io.*;
import java.util.List;

import org.junit.*;

public class TheadTest {
	public static class Runner extends Thread {
		public static volatile int BLACKHOLE = 0;
		
		public static void action() {
			for (int i = 0; i < 100000; i++) {
				BLACKHOLE += i;
			}
		}
		
		@Override
		public void run() {
			action();
		}
	}
	
	@Before
	public void warmUp() {
		for (int i = 0; i < 100; i++) {
			runThreads(12);
		}
	}
	
	@Test
	public void threadNoInheritance() throws IOException, InterruptedException {
		final String[] events = new String[] { "PAPI_TOT_INS" };
		Benchmark.init(2, events);
		
		Benchmark.start();
		runThreads(1);
		Benchmark.stop();
		
		Benchmark.start();
		runThreads(100);
		Benchmark.stop();
		

		List<long[]> results = Benchmark.getResults().getData();
		long resultOneThread = results.get(0)[0];
		long resultManyThreads = results.get(1)[0];
		
		double orderOfMagnitudeOneThread = Math.log10(resultOneThread);
		double orderOfMagnitudeManyThreads = Math.log10(resultManyThreads);
		
		Assert.assertEquals(orderOfMagnitudeOneThread, orderOfMagnitudeManyThreads, 0.25);
	}
	
	@Test
	public void threadInheritance() throws IOException, InterruptedException {
		final String[] events = new String[] { "PAPI_TOT_INS" };
		Benchmark.init(2, events, Benchmark.THREAD_INHERIT);
		
		Benchmark.start();
		runThreads(1);
		Benchmark.stop();
		
		Benchmark.start();
		runThreads(1000);
		Benchmark.stop();
		

		List<long[]> results = Benchmark.getResults().getData();
		long resultOneThread = results.get(0)[0];
		long resultManyThreads = results.get(1)[0];
		
		double orderOfMagnitudeOneThread = Math.log10(resultOneThread);
		double orderOfMagnitudeManyThreads = Math.log10(resultManyThreads);
		
		Assert.assertEquals(orderOfMagnitudeOneThread + 3, orderOfMagnitudeManyThreads, 1);
	}
	
	private void runThreads(int threadCount) {
		Thread[] threads = new Thread[threadCount];
		for (int i = 0; i < threads.length; i++) {
			threads[i] = new Runner();
		}
		
		for (Thread t : threads) {
			t.start();
		}
		
		for (int i = 0; i < 5; i++) {
			Runner.action();
		}
		
		for (Thread t : threads) {
			try {
				t.join();
			} catch (InterruptedException e) {
				
			}
		}
	}
	
}
