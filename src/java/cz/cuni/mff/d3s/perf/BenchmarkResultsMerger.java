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

public class BenchmarkResultsMerger implements BenchmarkResults {
	private List<String> events;
	private List<long[]> data;
	
	public BenchmarkResultsMerger() {
		events = new ArrayList<>();
		data = new ArrayList<>();
	}
	
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
	
	@Override
	public String[] getEventNames() {
		return events.toArray(new String[1]);
	}
	
	@Override
	public List<long[]> getData() {
		return Collections.unmodifiableList(data);
	}
}
