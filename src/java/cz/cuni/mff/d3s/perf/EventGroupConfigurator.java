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

import java.util.ArrayList;
import java.util.List;

/** Event set groups event definitions, end-user naming and attachment to a thread. */
public final class EventGroupConfigurator {
    private static final class Event {
        public final String eventName;
        public final String columnHeader;
        
        public Event(String name, String column) {
            eventName = name;
            columnHeader = column;
        }
    }
    
    /** Events of this event set. */
    private final List<Event> events = new ArrayList<>();
    
    /** Whether this event set should be inherited by spawned threads. */
    private boolean isSpawning = false;
    
    /** Whether this event set is attached to same threads.
     *
     * <p>
     * Note that this property can be true and yet the thread list may be
     * empty.
     * That is possible, for example, when attaching to JVMCI threads
     * on non-JVMCI-enabled platform. There, the list is empty and we do
     * not want to create any event set.
     */
    private boolean isAttached = false;
    
    /** Native thread ids to which this event set would be attached. */
    private final List<Long> threads = new ArrayList<>();
    
    /** Start building a new event set. */
    public EventGroupConfigurator() {}
    
    public static EventGroupConfigurator loadFromString(String eventList) {
        EventGroupConfigurator result = new EventGroupConfigurator();
        if (eventList == null) {
            return result;
        }
        
        return EventGroupConfigurator.loadFromArray(eventList.split(","));
    }
    
    public static EventGroupConfigurator loadFromArray(String[] events) {
        EventGroupConfigurator result = new EventGroupConfigurator();
        
        if (events == null) {
            return result;
        }
        
        for (String ev : events) {
            String[] parts = ev.trim().split("=", 2);
            parts[0] = parts[0].trim();
            if (parts[0].isEmpty()) {
                continue;
            }
            
            if (parts.length == 1) {
                result.addEvent(parts[0]);
            } else {
                parts[1] = parts[1].trim();
                if (parts[1].isEmpty()) {
                    parts[1] = parts[0];
                }
                result.addEvent(parts[0], parts[1]);
            }
        }
        
        return result;
    }
    
    public EventGroupConfigurator addEvent(String event) {
        return addEvent(event, event);
    }
    
    public EventGroupConfigurator addEvent(String event, String alias) {
        if (!Measurement.isEventSupported(event)) {
            throw new MeasurementException(String.format("Event %s not supported.", event));
        }
        
        events.add(new Event(event, alias));
        
        return this;
    }
    
    public EventGroupConfigurator setSpawning() {
        this.isSpawning = true;
        return this;
    }
    
    public EventGroupConfigurator attachTo(Thread targetThread) {
        isAttached = true;
        long nativeId = NativeThreads.getNativeId(targetThread);
        threads.add(nativeId);
        return this;
    }
    
    public EventGroupConfigurator attachToJvmCiThreads() {
        isAttached = true;
        for (Long threadId : JvmUtils.getNativeIdsOfJvmciThreads()) {
            threads.add(threadId);
        }
        return this;
    }
    
    public EventGroup create() {
        int measurementCount = 1;
        String[] eventNames = new String[events.size()];
        String[] columnNames = new String[events.size()];
        for (int i = 0; i < eventNames.length; i++) {
            eventNames[i] = events.get(i).eventName;
            columnNames[i] = events.get(i).columnHeader;
        }
        int[] eventSets;
        
        if (!isAttached) {
            int eventSet;
            if (isSpawning) {
                eventSet = Measurement.createEventSet(measurementCount, eventNames, Measurement.THREAD_INHERIT);
            } else {
                eventSet = Measurement.createEventSet(measurementCount, eventNames);
            }
            eventSets = new int[] { eventSet };
        } else {
            eventSets = new int[threads.size()];
            for (int i = 0; i < eventSets.length; i++) {
                if (isSpawning) {
                    eventSets[i] = Measurement.createAttachedEventSetOnNativeThread(threads.get(i), measurementCount, eventNames, Measurement.THREAD_INHERIT);
                } else {
                    eventSets[i] = Measurement.createAttachedEventSetOnNativeThread(threads.get(i), measurementCount, eventNames);
                }
            }
        }
        
        return new EventGroup(eventSets, columnNames);
    }
}
