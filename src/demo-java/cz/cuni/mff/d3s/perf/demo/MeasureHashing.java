/*
 * Copyright 2015 Charles University in Prague
 * Copyright 2015 Vojtech Horky
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

import cz.cuni.mff.d3s.perf.Benchmark;
import cz.cuni.mff.d3s.perf.BenchmarkResults;
import cz.cuni.mff.d3s.perf.BenchmarkResultsPrinter;
import cz.cuni.mff.d3s.perf.Measurement;

public class MeasureHashing {

    /** Whether to print the computed hash. */
    private static final boolean PRINT_HASH = true;

    /** File reading buffer size. */
    private static final int BUFFER_SIZE = 4096;

    /** Number of loops to perform if not specified at the command line. */
    private static final int DEFAULT_LOOPS = 30;

    /** Events to measure. */
    private static final String[] EVENTS = {
        "SYS:wallclock-time",
        "SYS:thread-time",
        "JVM:compilations",
        "PAPI_TOT_INS",
        "PAPI_L1_DCM"
    };

    /** Prevent dead-code elimination and other optimizations ;-). */
    public static volatile int BLACKHOLE = 0;

    public static void main(String[] args) throws NoSuchAlgorithmException, IOException {
        if (args.length == 0) {
            printUsage();
            System.exit(-1);
        }

        /*
         * Filename is the first argument,
         * number of loops (optional) is the second one.
         */
        String filename = args[0];
        int loops = DEFAULT_LOOPS;
        if (args.length > 1) {
            loops = Integer.parseInt(args[1]);
        }

        /*
         * Initialize all the data, that is the digest engine, benchmarking
         * and the input file.
         */
        Benchmark.init(loops, Measurement.filterSupportedEvents(EVENTS));
        MessageDigest hash = MessageDigest.getInstance("md5");
        FileInputStream input = new FileInputStream(filename);
        byte[] buffer = new byte[BUFFER_SIZE];

        /*
         * Run the benchmark in separate method to allow JIT compile
         * the whole method (shall be better than on-stack-replacement).
         */
        for (int i = 0; i < loops; i++) {
            action(input, hash, buffer);
        }

        input.close();

        /*
         * Get the results and print them as pseudo-table.
         */
        BenchmarkResults results = Benchmark.getResults();
        BenchmarkResultsPrinter.table(results, System.out);

        /*
         * For testing purposes, print the hash as well.
         */
        if (PRINT_HASH) {
            byte[] digest = hash.digest();
            System.out.printf("Hash is ");
            for (byte b : digest) {
                System.out.printf("%02x", b);
            }
            System.out.printf(".\n");
        }
    }

    private static void action(FileInputStream input, MessageDigest digest, byte[] buffer) throws IOException {
        /*
         * Reset the digest and rewind the file.
         * We do not want to measure this.
         */
        digest.reset();
        input.getChannel().position(0);

        /*
         * Inside the benchmark, read the file and update the hash.
         */
        Benchmark.start();
        while (true) {
            int bytesRead = input.read(buffer);
            if (bytesRead <= 0) {
                break;
            }
            digest.update(buffer, 0, bytesRead);
        }
        byte[] footprint = digest.digest();
        Benchmark.stop();

        /*
         * Prevent optimization (hopefully, this is enough.
         */
        BLACKHOLE += footprint[0];
    }

    private static void printUsage() {
        System.out.printf("Usage: java %s filename [loops]\n", MeasureHashing.class.getName());
    }

}
