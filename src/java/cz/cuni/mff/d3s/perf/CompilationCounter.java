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

/** Provides access to number of JIT compilations. */
public final class CompilationCounter {

    static {
        UbenchAgent.load();
    }

    /** Prevent instantiation. */
    private CompilationCounter() {}

    /** Get number of compilation counts since last call and resets the counter.
     *
     * @return Number of JIT compilations since last call.
     */
    public static native int getCompilationCountAndReset();
}
