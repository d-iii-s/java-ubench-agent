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

import java.util.*;

import org.junit.*;

@Ignore
public class ThreadTimeComparison {
	private static final int SAMPLES = 10;
	private static final int LOOPS = 100;
	
	static class BenchmarkData<T> {
		public int eventSet;
		public T diffs[];
		public int index = 0;
		
		@SuppressWarnings("unchecked")
		public BenchmarkData(String ... events) {
			eventSet = Measurement.createEventSet(1, events);
			diffs = (T[]) new Object[SAMPLES];
		}
		
		public void add(T value) {
			diffs[index] = value;
			index = (index + 1) % diffs.length;
		}
		
		@Override
		public String toString() {
			StringBuilder res = new StringBuilder();
			res.append(diffs[0]);
			for (int i = 1; i < diffs.length; i++) {
				res.append(", ");
				res.append(diffs[i]);
			}
			return res.toString();
		}
	}
	
	public static void main(String[] args) {
		List<BenchmarkData<Long>> data = new ArrayList<>(2);
		data.add(new BenchmarkData<Long>("SYS:wallclock-time", "SYS:thread-time"));
		data.add(new BenchmarkData<Long>("SYS:wallclock-time", "SYS:thread-time-rusage"));
		
		Random rnd = new Random();
		for (int i = 0; i < LOOPS; i++) {
			int index = rnd.nextInt(data.size());
			System.out.print(index);
			System.out.flush();
			
			BenchmarkData<Long> current = data.get(index);
			
			Measurement.start(current.eventSet);
			long endTime = System.currentTimeMillis() + 100;
			while (System.currentTimeMillis() < endTime) {
				// Busy wait.
			}
			Measurement.stop(current.eventSet);
			
			BenchmarkResults results = Measurement.getResults(current.eventSet);
			long values[] = results.getData().get(0);
			current.add(values[1] - values[0]);
		}
		System.out.println();
		
		System.out.printf("via clock_gettime(): %s\n", data.get(0));
		System.out.printf("via getrusage(): %s\n", data.get(1));
	}
}
