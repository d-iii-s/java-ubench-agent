/*
 * Copyright 2020 Charles University in Prague
 * Copyright 2020 Vojtech Horky
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

import java.util.List;

import org.junit.*;

public class MeasurementSamplingTest {
    private static final String[] EVENTS = {
            "SYS:wallclock-time",
    };

    private void action(int eventSet) {
        Measurement.start(eventSet);
        Measurement.sample(1, eventSet);
        Measurement.sample(2, eventSet);
        Measurement.sample(3, eventSet);
        Measurement.stop(eventSet);
    }

    @Test
    public void smoke() {
        int eventSet = Measurement.createEventSet(10, EVENTS);
        action(eventSet);
        action(eventSet);

        BenchmarkResults baseResults = Measurement.getResults(eventSet);
        BenchmarkResults rawResults = Measurement.getRawResults(eventSet);

        List<long[]> baseData = baseResults.getData();
        List<long[]> rawData = rawResults.getData();

        Assert.assertEquals(2, baseData.size());
        Assert.assertEquals(10, rawData.size());
    }

    private static final String[] RICH_EVENTS = {
            "SYS:wallclock-time",
            "SYS:forced-context-switches",
            "JVM:compilations",
            "PAPI_L1_DCM",
            "PAPI_L1_DCM",
    };
    private static final String[] SUPPORTED_EVENTS = Measurement.filterSupportedEvents(RICH_EVENTS);

    public static void main(String[] args) {
        int eventSet = Measurement.createEventSet(10, SUPPORTED_EVENTS);

        Measurement.start(eventSet);
        Measurement.sample(1, eventSet);
        Measurement.sample(2, eventSet);
        Measurement.sample(3, eventSet);
        Measurement.stop(eventSet);

        BenchmarkResultsPrinter.table(Measurement.getRawResults(eventSet), System.out);

        System.exit(0);
    }
}
