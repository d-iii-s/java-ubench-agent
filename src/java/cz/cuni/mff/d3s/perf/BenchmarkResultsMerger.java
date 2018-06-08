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

import java.util.*;

/** Helper class for merging multiple results. */
public class BenchmarkResultsMerger implements BenchmarkResults {
    /** Merged events. */
    private List<String> events;
    
    /** Merged data (row-wise). */
    private List<long[]> data;

    /** Default constructor. */
    public BenchmarkResultsMerger() {
        events = new ArrayList<>();
        data = new ArrayList<>();
    }

    /** Add extra results as new columns.
     *
     * @param results New results.
     * @param prefix Prefix of new columns after adding.
     */
    public void addColumns(BenchmarkResults results, String prefix) {
        List<long[]> newData = results.getData();
        if (events.size() != 0) {
            if (newData.size() != data.size()) {
                throw new IllegalArgumentException(String.format("Row number is different (wanted %d, but got %d).", data.size(), newData.size()));
            }
        }
        for (String ev : results.getEventNames()) {
            events.add(prefix + ev);
        }
        if (data.size() == 0) {
            for (int i = 0; i < newData.size(); i++) {
                data.add(new long[0]);
            }
        }
        for (int i = 0; i < data.size(); i++) {
            long[] newRow = newData.get(i);
            long[] existingRow = data.get(i);
            long[] updatedRow = Arrays.copyOf(existingRow, events.size());
            System.arraycopy(newRow, 0, updatedRow, existingRow.length, newRow.length);
            data.set(i, updatedRow);
        }
    }

    /** Add a new column.
     *
     * @param values Counter values.
     * @param name Column name.
     */
    public void addColumn(long[] values, String name) {
        if (events.size() != 0) {
            if (values.length != data.size()) {
                throw new IllegalArgumentException(String.format("Row number is different (wanted %d, but got %d).", data.size(), values.length));
            }
        }
        events.add(name);

        if (data.size() == 0) {
            for (int i = 0; i < values.length; i++) {
                data.add(new long[] { values[i] });
            }
        } else {
            int columnIndex = events.size() - 1;
            for (int i = 0; i < values.length; i++) {
                long[] updatedRow = Arrays.copyOf(data.get(i), columnIndex + 1);
                updatedRow[columnIndex] = values[i];
                data.set(i, updatedRow);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public String[] getEventNames() {
        return events.toArray(new String[1]);
    }

    /** {@inheritDoc} */
    @Override
    public List<long[]> getData() {
        return Collections.unmodifiableList(data);
    }
}
