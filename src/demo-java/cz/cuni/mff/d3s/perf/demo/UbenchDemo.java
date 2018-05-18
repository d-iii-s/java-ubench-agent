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

import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

import cz.cuni.mff.d3s.perf.Benchmark;
import cz.cuni.mff.d3s.perf.BenchmarkResults;
import cz.cuni.mff.d3s.perf.BenchmarkResultsMerger;
import cz.cuni.mff.d3s.perf.BenchmarkResultsPrinter;
import cz.cuni.mff.d3s.perf.Measurement;

public class UbenchDemo {
	/** File reading buffer size. */
	private static final int BUFFER_SIZE = 4096;
	
	/** Number of loops to perform if not specified at the command line. */
	private static final int DEFAULT_LOOPS = 10;
	
	/** Events to measure if not specified at the command line. */
	private static final String[] DEFAULT_EVENTS = {
		"SYS:wallclock-time",
		//"SYS:thread-time",
		//"JVM:compilations"
	};
	
	/** Prevent dead-code elimination and other optimizations ;-). */
	public static volatile int BLACKHOLE = 0;

	public static void main(String[] args) throws NoSuchAlgorithmException, IOException {
		String inputFilename = null;
		int loops = DEFAULT_LOOPS;
		List<String[]> events = new LinkedList<>();
		
		for (String arg : args) {
			if (arg.startsWith("--loops=")) {
				loops = Integer.parseInt(arg.substring("--loops=".length()));
				if (loops <= 0) {
					printUsage();
					System.exit(1);
				}
			} else if (arg.startsWith("--events=")) {
				String[] ev = arg.substring("--events=".length()).split(",");
				for (String e : ev) {
					if (!Measurement.isEventSupported(e)) {
						System.err.printf("Unknown event `%s'.\n", e);
						System.exit(1);
					}
				}
				events.add(ev);
			} else {
				if (inputFilename != null) {
					printUsage();
					System.exit(1);
				}
				inputFilename = arg;
			}
		}
		
		if (inputFilename == null) {
			printUsage();
			System.exit(1);
		}
		
		if (events.isEmpty()) {
			events.add(DEFAULT_EVENTS);
		}
		
		
		/*
		 * Initialize the digest engine and the input file.
		 */
		MessageDigest hash = MessageDigest.getInstance("md5");
		FileInputStream input = new FileInputStream(inputFilename);
		byte[] buffer = new byte[BUFFER_SIZE];

		/*
		 * Initialize the event sets.
		 */
		int[] eventSets = new int[events.size()];
		for (int i = 0; i < eventSets.length; i++) {
			eventSets[i] = Measurement.createEventSet(loops, events.get(i));
		}
		
		/*
		 * Run the benchmark in separate method to allow JIT compile
		 * the whole method (shall be better than on-stack-replacement).
		 */
		for (int i = 0; i < loops; i++) {
			action(input, hash, buffer, eventSets);
		}
		
		input.close();
		
		/*
		 * Get the results and print them as pseudo-table.
		 */
		BenchmarkResultsMerger results = new BenchmarkResultsMerger();
		for (int ev : eventSets) {
			BenchmarkResults res = Measurement.getResults(ev);
			results.addColumns(res, "");
		}
		BenchmarkResultsPrinter.table(results, System.out);
	}
	
	private static void action(FileInputStream input, MessageDigest digest, byte[] buffer, int[] eventSets) throws IOException {
		/*
		 * Reset the digest and rewind the file.
		 * We do not want to measure this.
		 */
		digest.reset();
		input.getChannel().position(0);

		/*
		 * Inside the benchmark, read the file and update the hash.
		 */
		Measurement.start(eventSets);
		while (true) {
			int bytesRead = input.read(buffer);
			if (bytesRead <= 0) {
				break;
			}
			digest.update(buffer, 0, bytesRead);
		}
		byte[] footprint = digest.digest();
		Measurement.stop(eventSets);
		
		/*
		 * Prevent optimization (hopefully, this is enough).
		 */
		BLACKHOLE += footprint[0]; 
	}

	private static void printUsage() {
		System.out.printf("Usage: java %s filename [--loops=LOOPS] [--events=EV1,EV2] [...]\n", UbenchDemo.class.getName());
	}

}
