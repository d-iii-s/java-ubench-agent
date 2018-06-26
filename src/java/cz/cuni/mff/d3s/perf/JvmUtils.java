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

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashSet;
import java.util.NoSuchElementException;
import java.util.Scanner;
import java.util.Set;

/** Gather information about JVM. */
public final class JvmUtils {
    /** Prevent instantiation. */
    private JvmUtils() {}

    /** Get list of native ids of threads related to JVMCI.
     *
     * <p>
     * <b>Warning:</b> This method works on best effort basis and it may
     * not always return a complete list due to differences between
     * OSes and individual JVMs.
     *
     * <p>
     * <b>Warning:</b> This method uses Thread.getAllStackTraces() to
     * retrieve list of all threads (as iterating through top-most
     * thread group does not give a full list). This method has behavior
     * of (almost) stop-the-world. Therefore, do not invoke this method
     * very often and never in performance-critical sections of your
     * code.
     *
     * @return List of native thread ids.
     */
    public static Set<Long> getNativeIdsOfJvmciThreads() {
        Set<Long> result = new HashSet<>();

        for (Thread t : Thread.getAllStackTraces().keySet()) {
            if (t.getName().startsWith("JVMCI CompilerThread")) {
                try {
                    long nativeId = NativeThreads.getNativeId(t);
                    result.add(nativeId);
                } catch (NoSuchElementException e) {
                    // Okay, the thread is not known, we will try some
                    // other way later on.
                }
            }
        }

        linuxGetSelfThreadsStartingWith("JVMCI CompilerT", result);

        return result;
    }

    /** Get list of native ids of threads starting with given prefix.
     *
     * <p>
     * The prefix is tested on native names as found in
     * <code>/proc/nnnn/status</code> file.
     *
     * @param prefix Native thread name prefix.
     * @param pids Where to store the found ids.
     */
    private static void linuxGetSelfThreadsStartingWith(final String prefix, final Set<Long> pids) {
        File tasks = new File("/proc/self/task");
        File[] threadDirs = tasks.listFiles();
        if (threadDirs == null) {
            return;
        }

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
                        if (name.startsWith(prefix)) {
                            pids.add(id);
                        }
                        break;
                    }
                }
                sc.close();
            } catch (IOException e) {
                continue;
            }
        }
    }

    /** Checks that the native agent is actually attached to the JVM.
     *
     * @return Whether the native part of this library is available.
     */
    public static boolean isNativeAgentAvailable() {
        try {
            Measurement.isEventSupported("nonsense");
            return true;
        } catch (UnsatisfiedLinkError e) {
            return false;
        }
    }
}
