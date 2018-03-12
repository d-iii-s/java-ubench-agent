/*
 * Copyright 2018 Charles University in Prague
 * Copyright 2018 Vojtech Horky
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
package cz.cuni.mff.d3s.perf.demo;

import java.util.ArrayList;
import java.util.List;

import cz.cuni.mff.d3s.perf.BenchmarkResults;
import cz.cuni.mff.d3s.perf.Measurement;
import cz.cuni.mff.d3s.perf.MeasurementException;

public class MeasureJVMCIThreads {
	private static final String[] EVENTS = { "PAPI_TOT_INS" };

	private static List<Thread> getAllThreads() {
		List<Thread> threads = new ArrayList<>();
		threads.addAll(Thread.getAllStackTraces().keySet());
		return threads;
	}

	private static Thread[] getJVMCIThreads(List<Thread> all) {
		List<Thread> res = new ArrayList<>(all.size());
		for (Thread t : all) {
			if (t.getName().startsWith("JVMCI CompilerThread")) {
				res.add(t);
			}
		}
		return res.toArray(new Thread[0]);
	}

	private static long action() {
		long res = 0;
		for (int i = 0; i < 1000; i++) {
			res += System.nanoTime();
		}
		return res;
	}

	public static void main(String[] args) {
		Thread[] jvmciThreads = getJVMCIThreads(getAllThreads());
		if (jvmciThreads.length == 0) {
			System.out.printf("No JVMCI threads found.\n");
			return;
		}

		int[] eventSets = new int[jvmciThreads.length];
		for (int i = 0; i < jvmciThreads.length; i++) {
			eventSets[i] = -1;
			try {
				eventSets[i] = Measurement.createAttachedEventSet(jvmciThreads[i], 1, EVENTS);
				System.out.printf("Thread %s got event set %d.\n", jvmciThreads[i].getName(), eventSets[i]);
			} catch (MeasurementException e) {
				System.out.printf("Skipping thread %s [%s].\n", jvmciThreads[i].getName(), e.getMessage());
			}

		}

		for (int i = 0; i < eventSets.length; i++) {
			if (eventSets[i] >= 0) {
				Measurement.start(eventSets[i]);
			}
		}

		long blackhole = 0;
		for (int i = 0; i < 1000; i++) {
			blackhole += action();
		}

		for (int i = 0; i < eventSets.length; i++) {
			if (eventSets[i] >= 0) {
				Measurement.stop(eventSets[i]);
			}
		}

		System.out.printf("Blackhole is %d.\n", blackhole);

		for (int i = 0; i < eventSets.length; i++) {
			if (eventSets[i] < 0) {
				continue;
			}

			BenchmarkResults res = Measurement.getResults(eventSets[i]);
			System.out.printf("Thread %s results:\n", jvmciThreads[i].getName());
			for (long[] row : res.getData()) {
				System.out.printf("   %d\n", row[0]);
			}

			Measurement.destroyEventSet(eventSets[i]);
		}
	}
}
