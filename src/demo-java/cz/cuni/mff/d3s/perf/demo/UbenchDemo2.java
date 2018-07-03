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
package cz.cuni.mff.d3s.perf.demo;

import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import cz.cuni.mff.d3s.perf.EventGroupConfigurator;
import cz.cuni.mff.d3s.perf.JvmUtils;
import cz.cuni.mff.d3s.perf.PerformanceMultiMeter;
import cz.cuni.mff.d3s.perf.PerformanceMultiMeterConfigurator;
import cz.cuni.mff.d3s.perf.TextResultsWriter;

public class UbenchDemo2 {
    /** File reading buffer size. */
    private static final int BUFFER_SIZE = 4096;

    /** Number of loops to perform if not specified at the command line. */
    private static final int DEFAULT_LOOPS = 10;

    /** Prevent dead-code elimination and other optimizations ;-). */
    public static volatile int BLACKHOLE = 0;

    public static void main(String[] args) throws NoSuchAlgorithmException, IOException {
        String inputFilename = null;
        int loops = DEFAULT_LOOPS;
        
        /*
         * Check that the native agent is attached.
         */
        if (!JvmUtils.isNativeAgentAvailable()) {
            System.err.println("Error: the native agent seems not to be attached");
            System.err.println("to the current JVM, aborting.");
            System.exit(1);
        }
        
        PerformanceMultiMeterConfigurator configurator = PerformanceMultiMeter.configure()
                .setWriter(TextResultsWriter.tabular(System.out))
                .loadFromProperties();
        
        for (String arg : args) {
            if (arg.startsWith("--loops=")) {
                loops = Integer.parseInt(arg.substring("--loops=".length()));
                if (loops <= 0) {
                    printUsage();
                    System.exit(1);
                }
            } else if (arg.startsWith("--events=")) {
                String events = arg.substring("--events=".length());
                EventGroupConfigurator eventSet = EventGroupConfigurator.loadFromString(events);
                configurator.addEventGroup(eventSet.create());
            } else if (arg.equals("--csv")) {
                configurator.setWriter(TextResultsWriter.csv(System.out));
            } else {
                if (inputFilename != null) {
                    printUsage();
                    System.exit(1);
                }
                inputFilename = arg;
            }
        }

        if (inputFilename == null) {
            printUsage();
            System.exit(1);
        }


        /*
         * Initialize the digest engine and the input file.
         */
        MessageDigest hash = MessageDigest.getInstance("md5");
        FileInputStream input = new FileInputStream(inputFilename);
        byte[] buffer = new byte[BUFFER_SIZE];

        /*
         * Initialize the performance meter.
         */
        PerformanceMultiMeter meter = configurator.create();
        meter.selfTest();

        /*
         * Run the benchmark in separate method to allow JIT compile
         * the whole method (shall be better than on-stack-replacement).
         */
        for (int i = 0; i < loops; i++) {
            action(input, hash, buffer, meter);
        }

        input.close();

        meter.saveResults();
    }

    private static void action(FileInputStream input, MessageDigest digest, byte[] buffer, PerformanceMultiMeter meter) throws IOException {
        /*
         * Reset the digest and rewind the file.
         * We do not want to measure this.
         */
        digest.reset();
        input.getChannel().position(0);

        /*
         * Inside the benchmark, read the file and update the hash.
         */
        meter.start();
        while (true) {
            int bytesRead = input.read(buffer);
            if (bytesRead <= 0) {
                break;
            }
            digest.update(buffer, 0, bytesRead);
        }
        byte[] footprint = digest.digest();
        meter.stop();

        /*
         * Prevent optimization (hopefully, this is enough).
         */
        BLACKHOLE += footprint[0];
    }

    private static void printUsage() {
        System.out.printf("Usage: java %s filename [--loops=LOOPS] [--events=EV1,EV2] [...]\n", UbenchDemo2.class.getName());
    }

}
