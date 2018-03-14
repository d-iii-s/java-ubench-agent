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

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

import cz.cuni.mff.d3s.perf.BenchmarkResults;
import cz.cuni.mff.d3s.perf.Measurement;
import cz.cuni.mff.d3s.perf.MeasurementException;

public class MeasureJVMCIThreads {
	private static final String[] EVENTS = { "PAPI_TOT_INS" };

	private static class ThreadWrapper {
		private final Thread javaThread;
		private final long nativeThreadId;
		private final String name;

		private ThreadWrapper(Thread javaThread, long nativeThreadId, String nativeThreadName) {
			this.javaThread = javaThread;
			this.nativeThreadId = nativeThreadId;
			this.name = nativeThreadName;
		}

		public static ThreadWrapper createJavaThread(Thread thread) {
			return new ThreadWrapper(thread, -1, thread.getName());
		}

		public static ThreadWrapper createNativeThread(long id, String name) {
			return new ThreadWrapper(null, id, name);
		}

		public boolean isJavaThread() {
			return javaThread != null;
		}

		public String getName() {
			return name;
		}

		public long getNativeThreadId() {
			if (isJavaThread()) {
				throw new IllegalStateException("This is a Java thread.");
			}
			return nativeThreadId;
		}

		public Thread getJavaThread() {
			if (!isJavaThread()) {
				throw new IllegalStateException("This is a native thread.");
			}
			return javaThread;
		}
	}

	private static ThreadWrapper[] getJVMCIThreads() {
		List<ThreadWrapper> result = new ArrayList<>();

		for (Thread t : Thread.getAllStackTraces().keySet()) {
			if (t.getName().startsWith("JVMCI CompilerThread")) {
				result.add(ThreadWrapper.createJavaThread(t));
			}
		}

		File tasks = new File("/xproc/self/task");
		File[] threadDirs = tasks.listFiles();
		for (File threadDir : threadDirs) {
			try {
				long id = Long.parseLong(threadDir.getName());
				FileReader reader = new FileReader(new File(threadDir, "status"));
				Scanner sc = new Scanner(reader);
				while (sc.hasNextLine()) {
					String line = sc.nextLine();
					String[] parts = line.split("\t", 2);
					if (parts.length != 2) {
						continue;
					}
					if (parts[0].equals("Name:")) {
						String name = parts[1];
						if (name.startsWith("JVMCI CompilerT")) {
							result.add(ThreadWrapper.createNativeThread(id, name));
						}
						break;
					}
				}
				sc.close();
			} catch (IOException e) {
				continue;
			}
		}

		return result.toArray(new ThreadWrapper[0]);
	}

	private static long action() {
		long res = 0;
		for (int i = 0; i < 1000; i++) {
			res += System.nanoTime();
		}
		return res;
	}

	public static void main(String[] args) {
		ThreadWrapper[] jvmciThreads = getJVMCIThreads();
		if (jvmciThreads.length == 0) {
			System.out.printf("No JVMCI threads found.\n");
			return;
		}

		int[] eventSets = new int[jvmciThreads.length];
		for (int i = 0; i < jvmciThreads.length; i++) {
			eventSets[i] = -1;
			ThreadWrapper t = jvmciThreads[i];
			try {
				if (t.isJavaThread()) {
					eventSets[i] = Measurement.createAttachedEventSet(t.getJavaThread(), 1, EVENTS);
				} else {
					eventSets[i] = Measurement.createAttachedEventSetOnNativeThread(t.getNativeThreadId(), 1, EVENTS);
				}
				System.out.printf("Thread %s got event set %d.\n", t.getName(),	eventSets[i]);
			} catch (MeasurementException e) {
				System.out.printf("Skipping thread %s [%s].\n", jvmciThreads[i].getName(),
						e.getMessage());
			}

		}

		for (int i = 0; i < eventSets.length; i++) {
			if (eventSets[i] >= 0) {
				Measurement.start(eventSets[i]);
			}
		}

		long blackhole = 0;
		long end = System.currentTimeMillis() + 5 * 1000;
		while (System.currentTimeMillis() < end) {
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
