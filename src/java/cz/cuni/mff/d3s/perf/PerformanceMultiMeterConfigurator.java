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

import java.io.FileNotFoundException;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;

/** Swiss-army knife of performance measuring. */
public class PerformanceMultiMeterConfigurator {
    
    private class NullStorage implements ResultsWriter {

        @Override
        public void writeHeader(final String[] names) {}

        @Override
        public void writeRow(final long[] values) {}
    }
    
    private class NullPerformanceMultiMeter extends PerformanceMultiMeter {

        public NullPerformanceMultiMeter(ResultsWriter storage) {
            super(storage, new EventGroup[0]);
        }
        
        @Override
        public void start() {}
        
        @Override
        public void stopMeasurement() {}
    }
    
    private final List<EventGroup> eventGroups = new ArrayList<>();
    private ResultsWriter storage = new NullStorage();
    
    public PerformanceMultiMeterConfigurator() {
    }
    
    public PerformanceMultiMeter create() {
        EventGroup[] events = eventGroups.toArray(new EventGroup[0]);
        if (events.length == 0) {
            return new NullPerformanceMultiMeter(storage);
        } else {
            return new PerformanceMultiMeter(storage, events);
        }
    }
    
    public PerformanceMultiMeterConfigurator loadFromProperties() {
        return loadFromProperties("d3s.benchmark.");
    }
    
    public PerformanceMultiMeterConfigurator loadFromProperties(String propertyPrefix) {
        String[] eventSetNames = getEventSetOrderingFromProperties(propertyPrefix);
        
        for (String eventSetName : eventSetNames) {
            String property = propertyPrefix + "eventset." + eventSetName;
            
            String eventSetDescription = System.getProperty(property);
            if (eventSetDescription == null) {
                throw new MeasurementException("Event set '" + eventSetName + "' not found.");
            }
            
            addFromString(eventSetDescription);
        }
        
        String outputProperty = propertyPrefix + "output";
        String outputFilename = System.getProperty(outputProperty);
        if (outputFilename != null) {
            PrintStream output;
            try {
                output = new PrintStream(outputFilename);
                this.setWriter(TextResultsWriter.csv(output));
            } catch (FileNotFoundException e) {
                throw new MeasurementException(String.format(
                        "Cannot write to output `%s' (set via property `%s').",
                        outputFilename, outputProperty));
            }
        }
        
        return this;
    }
    
    public PerformanceMultiMeterConfigurator addEventGroup(final EventGroup eventSet) {
        eventGroups.add(eventSet);
        return this;
    }
    
    
    public PerformanceMultiMeterConfigurator setWriter(ResultsWriter writer) {
        storage = writer;
        return this;
    }
    
    private void addFromString(String desriptor) {
        String[] parts = desriptor.split("@", 2);
        
        String[] flags;
        String events;
        if (parts.length == 1) {
            events = parts[0];
            flags = new String[0];
        } else {
            flags = parts[0].split(",");
            events = parts[1];
        }
        
        EventGroupConfigurator eventGroup = EventGroupConfigurator.loadFromString(events);
        
        for (String flag : flags) {
            if (flag.equals("inherit") || flag.equals("spawn")) {
                eventGroup.setSpawning();
            } else if (flag.equals("jvmci")) {
                eventGroup.attachToJvmCiThreads();
            } else if (flag.isEmpty()) {
                // Empty flag is silently skipped.
            } else {
                throw new MeasurementException("Unknown flag '" + flag + "'.");
            }
        }
        
        this.addEventGroup(eventGroup.create());
    }
    
    private static String[] getEventSetOrderingFromProperties(String prefix) {
        String ordering = System.getProperty(prefix + "eventsetorder");
        if (ordering == null) {
            String eventSetsPrefix = prefix + "eventset.";
            Set<String> allPropertyNames = System.getProperties().stringPropertyNames();
            List<String> result = new ArrayList<>();
            for (String property : allPropertyNames) {
                if (property.startsWith(eventSetsPrefix)) {
                    result.add(property.substring(eventSetsPrefix.length()));
                }
            }
            Collections.sort(result);
            return result.toArray(new String[0]);
        } else {
            return ordering.split(",");
        }
    }
}
