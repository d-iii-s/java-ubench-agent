/*
 * Copyright 2017 Charles University in Prague
 * Copyright 2017 Vojtech Horky
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

@Ignore
public class BarrierTest {
	public static void main(String[] args) {
		if (args.length != 2) {
			System.err.println("Run with 2 arguments: program name and shared memory identifier.");
			System.exit(1);
		}
		
		String selfName = args[0];
		String barrierName = args[1];
		
		Barrier.init(barrierName);
		System.out.printf("%s: barrier %s initialized.\n", selfName, barrierName);
		
		System.out.printf("%s: waiting on barrier...\n", selfName);
		Barrier.barrier();
		
		System.out.printf("%s: got past the barrier, terminating.\n", selfName);
	}
}
