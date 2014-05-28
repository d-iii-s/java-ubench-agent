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

public class CompilationCounterTest {
	public static volatile long BLACK_HOLE = 0;
	private static final int LOOPS_TO_ENSURE_COMPILATION = 10000;
	
	public static void main(String[] args) {
		boolean interpretedMode = isInterpretedMode(args);
		
		BLACK_HOLE = getCompilationCount();
		
		for (int i = 0; i < LOOPS_TO_ENSURE_COMPILATION; i++) {
			BLACK_HOLE = action(i);
		}
		
		int events = getCompilationCount();
		
		boolean okay = true;
		if (interpretedMode && (events > 0)) {
			System.err.printf("Expected no compilation events in interpreted mode but got %d.\n", events);
			okay = false;
		} else if (!interpretedMode && (events == 0)) {
			System.err.printf("Expected at least one compilation event in mixed mode.\n");
			okay = false;
		}
		
		System.exit(okay ? 0 : 1);
	}
	
	private static int getCompilationCount() {
		int result = CompilationCounter.getCompilationCountAndReset();
		if (result < 0) {
			System.err.printf("Internal error occured, counter is negative (%d).\n", result);
			System.exit(2);
		}
		return result;
	}
	
	private static boolean isInterpretedMode(String[] programArgs) {
		if (programArgs.length == 0) {
			return false;
		}
		return programArgs[0].equalsIgnoreCase("int");
	}
	
	private static long action(int counter) {
		long result = counter + System.nanoTime();
		System.out.printf("");
		return result;
	}
}
