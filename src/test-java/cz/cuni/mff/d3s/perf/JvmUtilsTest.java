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
package cz.cuni.mff.d3s.perf;

import java.io.FileReader;
import java.io.IOException;
import java.util.Scanner;
import java.util.Set;
import java.util.regex.Pattern;

import org.junit.*;

public class JvmUtilsTest {
    private static final Pattern NAME_MATCHES_JVMCI_THREAD = Pattern.compile("^Name:.*JVMCI.*", Pattern.CASE_INSENSITIVE);

    @Test
    public void jvmciThreadsFound() {
        Assume.assumeTrue(System.getProperty("java.vm.version", "").contains("jvmci"));

        Set<Long> tids = JvmUtils.getNativeIdsOfJvmciThreads();

        Assert.assertFalse(tids.isEmpty());
    }

    @Test
    public void jvmciThreadsFoundOnLinux() throws IOException {
        // Skipping as it does not work this way on all JDKs
        Assume.assumeTrue(false);

        Set<Long> tids = JvmUtils.getNativeIdsOfJvmciThreads();

        Assume.assumeTrue(System.getProperty("os.name", "").equals("Linux"));
        for (Long tid : tids) {
            String path = String.format("/proc/%d/status", tid);
            Scanner sc = new Scanner(new FileReader(path));
            boolean found = false;
            while (sc.hasNextLine()) {
                String line = sc.nextLine();
                System.out.println(line);
                if (NAME_MATCHES_JVMCI_THREAD.matcher(line).find()) {
                    found = true;
                    break;
                }
            }
            sc.close();

            Assert.assertTrue(
                    String.format("Thread %d seems not to be a JVMCI thread.", tid),
                    found
            );
        }
    }
}
