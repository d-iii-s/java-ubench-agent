/*
 * Copyright 2018 Charles University
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

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/** Swiss-army knife of performance measurement.
 *
 * <p>
 * Use the configure() method to create fluent API configurator of
 * this class to add event sets etc.
 *
 */
public class PerformanceMultiMeter {
    private ResultsWriter storage;
    
    private final List<EventGroup> eventGroups = new ArrayList<>();
    private final int[] eventSetIds;
    private final List<long[]> values = new ArrayList<>();
    private final String[] eventNames;
    
    public static PerformanceMultiMeterConfigurator configure() {
        return new PerformanceMultiMeterConfigurator();
    }
    
    public PerformanceMultiMeter(ResultsWriter storage, EventGroup... eventGroups) {
        int totalEventSetIds = 0;
        int totalEventNames = 0;
        for (EventGroup ev : eventGroups) {
            this.eventGroups.add(ev);
            totalEventSetIds += ev.getInternalEventIds().length;
            totalEventNames += ev.getEventNames().length;
        }
        eventSetIds = new int[totalEventSetIds];
        eventNames = new String[totalEventNames];
        int idIndex = 0;
        int namesIndex = 0;
        for (EventGroup ev : eventGroups) {
            for (int id : ev.getInternalEventIds()) {
                eventSetIds[idIndex] = id;
                idIndex++;
            }
            for (String name : ev.getEventNames()) {
                eventNames[namesIndex] = name;
                namesIndex++;
            }
        }
        this.storage = storage;
    }
    
    public void start() {
        Measurement.start(eventSetIds);
    }
    
    public void stopMeasurement() {
        Measurement.stop(eventSetIds);
    }
    
    public void processLastMeasurement() {
        long[] row = new long[eventNames.length];
        int index = 0;
        List<long[]> subresults = new ArrayList<>();
        for (EventGroup ev : eventGroups) {
            subresults.clear();
            for (int id : ev.getInternalEventIds()) {
                BenchmarkResults res = Measurement.getResults(id);
                subresults.add(res.getData().get(0));
            }
            ev.processData(subresults, row, index);
            index += ev.getEventNames().length;
        }
        values.add(row);
    }
    
    public void stop() {
        stopMeasurement();
        processLastMeasurement();
    }
        
    public void saveResults() throws IOException {
        storage.writeHeader(eventNames);
        for (long[] row : values) {
            storage.writeRow(row);
        }
    }
    
    public void selfTest() {
        start();
        stop();
        
        values.clear();
        // ...
    }
}
