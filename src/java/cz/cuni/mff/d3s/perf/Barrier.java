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

/** Process-wide barrier, Linux only now. */
public final class Barrier {
    /** Prevent instantiation. */
    private Barrier() {}

    /** Create a new barrier.
     *
     * @param name Barrier name.
     */
    public static synchronized void init(final String name) {
        initNative("java-ubench-agent" + name);
    }

    /** Actual interface for creating the barrier in C agent.
     *
     * @param name Barrier name.
     */
    private static native void initNative(String name);

    /** Waits on the barrier. */
    public static native void barrier();
}
