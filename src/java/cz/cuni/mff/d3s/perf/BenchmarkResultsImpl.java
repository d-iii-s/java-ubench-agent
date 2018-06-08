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

import java.util.*;

/** Simplest implementation of BenchmarkResults used by C agent. */
class BenchmarkResultsImpl implements BenchmarkResults {
    /** Collected events. */
    private final String[] events;
    
    /** Collected data (row-wise). */
    private final List<long[]> data;

    /** Construct with given columns.
     *
     * @param eventNames Event names (column headers).
     */
    public BenchmarkResultsImpl(String[] eventNames) {
        events = eventNames;
        data = new ArrayList<>();
    }

    /** Add one data row.
     *
     * @param row Counter values.
     */
    void addDataRow(long[] row) {
        /* Need to create the copy. */
        data.add(Arrays.copyOf(row, row.length));
    }

    /** {@inheritDoc} */
    @Override
    public String[] getEventNames() {
        return events;
    }

    /** {@inheritDoc} */
    @Override
    public List<long[]> getData() {
        return Collections.unmodifiableList(data);
    }
}
