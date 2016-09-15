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

import java.io.PrintStream;

public class BenchmarkResultsPrinter {
	public static void toCsv(BenchmarkResults results, PrintStream stream) {
		toCsv(results, stream, "\t", true);
	}
	
	public static void toCsv(BenchmarkResults results, PrintStream stream, String separator, boolean printHeader) {
		if (printHeader) {
			String[] eventNames = results.getEventNames();
			for (int i = 0; i < eventNames.length; i++) {
				if (i > 0) {
					stream.append(separator);
				}
				stream.append(eventNames[i]);
			}
			stream.append("\n");
		}
		
		for (long[] row : results.getData()) {
			for (int i = 0; i < row.length; i++) {
				if (i > 0) {
					stream.append(separator);
				}
				stream.append(String.format("%d", row[i]));
			}
			stream.append("\n");
		}
	}
	
	public static void table(BenchmarkResults results, PrintStream stream) {
		table(results, stream, 15, true);
	}
	
	public static void table(BenchmarkResults results, PrintStream stream, int columnWidth, boolean printHeader) {
		if (printHeader) {
			String format = String.format("%%%ds", columnWidth);
			for (String name : results.getEventNames()) {
				String nameBeginning = name.substring(0, Math.min(name.length(), columnWidth - 1));
				stream.append(String.format(format, nameBeginning));
			}
			stream.append("\n");
		}
		
		String format = String.format("%%%dd", columnWidth);
		for (long[] row : results.getData()) {
			for (long r : row) {
				stream.append(String.format(format, r));
			}
			stream.append("\n");
		}
	}
	
}
