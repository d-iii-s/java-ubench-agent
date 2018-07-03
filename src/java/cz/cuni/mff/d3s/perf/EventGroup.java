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

import java.util.Arrays;
import java.util.List;

/** Event group bounds together event names and internal C representation of the events.
 *
 * <p>
 * It is expected that the user will use EventGroupConfigurator to create event
 * groups.
 * Typically, event group would be passed to PerformanceMultiMeterConfigurator and not
 * handled by the user in any way.
 */
public final class EventGroup {
    /** Internal event set ids. */
    private final int[] ids;
    
    /** Event names.
     *
     * <p>
     * Note that length of this array will probably differ from length
     * of ids array.
     */
    private final String[] names;
    
    /** Private constructor to prevent instantiation. */
    @SuppressWarnings("unused")
    private EventGroup() {
        ids = new int[0];
        names = new String[0];
    }
    
    /** Package-private constructor to prevent instantiation outside of EventGroupConfigurator.
     *
     * @param ids Internal event set ids.
     * @param names Event names.
     */
    EventGroup(final int[] ids, final String[] names) {
        this.ids = Arrays.copyOf(ids, ids.length);
        this.names = Arrays.copyOf(names, names.length);
    }

    /** Get list of internal event set ids.
     *
     * @return List of internal event set ids.
     */
    public int[] getInternalEventIds() {
        return ids;
    }
    
    /** Get list of event names.
     *
     * @return List of event names (most often column names).
     */
    public String[] getEventNames() {
        return names;
    }
     
    public long[] processData(final List<long[]> row) {
        long[] result = new long[names.length];
        processData(row, result, 0);
        return result;
    }
    
    public void processData(final List<long[]> row, final long[] result, final int startIndex) {
        for (int i = startIndex; i < startIndex + names.length; i++) {
            result[i] = 0;
        }
        
        int offset = 0;
        for (long[] data : row) {
            for (long sample : data) {
                result[startIndex + offset] += sample;
                offset = (offset + 1) % names.length;
            }
        }
    }
}
