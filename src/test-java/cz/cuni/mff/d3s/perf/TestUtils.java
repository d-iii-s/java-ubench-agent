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

import java.io.*;
import java.util.*;

import org.junit.*;

public class TestUtils {
	
	public static void runInJvm(boolean exitCodeShallBeZero, String[] jvmArgs, String classname, String[] appArgs) throws IOException, InterruptedException {
		List<String> cmdline = new LinkedList<>();
		cmdline.add("java");
		
		String extraClasspath = System.getProperty("ubench.classpath");
		if (extraClasspath != null) {
			cmdline.add("-cp");
			cmdline.add(extraClasspath);
		}
		
		String agentPath = System.getProperty("ubench.agent");
		Assert.assertNotNull("Specify the agent filename via -Dubench.agent=",
			agentPath);;
		
		cmdline.add("-agentpath:" + agentPath);
		for (String arg : jvmArgs) {
			cmdline.add(arg);
		}
		
		cmdline.add(classname);
		for (String arg : appArgs) {
			cmdline.add(arg);
		}
		
		Process proc;
		proc = Runtime.getRuntime().exec(cmdline.toArray(new String[0]));
		int rc = proc.waitFor();
		
		if (exitCodeShallBeZero) {
			Assert.assertEquals(classname + " ought to exit with 0", 0, rc);
		} else {
			Assert.assertFalse(classname + " ought to exit with error", rc == 0);
		}
	}
}
