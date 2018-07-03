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

import java.io.PrintStream;

/** Provides several text-based implementations of ResultsWriter. */
public final class TextResultsWriter {

    public static class CsvOutput implements ResultsWriter {
        private final PrintStream output;
        
        public CsvOutput(final PrintStream output) {
            this.output = output;
        }
        
        @Override
        public void writeHeader(final String[] names) {
            for (int i = 0; i < names.length; i++) {
                if (i > 0) {
                    output.print(",");
                }
                output.print(names[i]);
            }
            output.println();
        }

        @Override
        public void writeRow(final long[] values) {
            for (int i = 0; i < values.length; i++) {
                if (i > 0) {
                    output.print(",");
                }
                output.print(values[i]);
            }
            output.println();
        }
    }
    
    
    public static class TabularOutput implements ResultsWriter {
        /** Minimum column width is set to 8 hours in nanoseconds. */
        private static final int MINIMUM_COLUMN_WIDTH = 14;

        private String[] columnFormats = new String[0];

        private final PrintStream output;

        public TabularOutput(final PrintStream output) {
            this.output = output;
        }

        @Override
        public void writeHeader(final String[] names) {
            columnFormats = new String[names.length];
            for (int i = 0; i < names.length; i++) {
                int width = Math.max(MINIMUM_COLUMN_WIDTH, names[i].length()) + 1;
                columnFormats[i] = makeFormatSpecifier(width, "d");
                output.printf(makeFormatSpecifier(width, "s"), names[i]);
            }
            output.println();
        }

        @Override
        public void writeRow(final long[] values) {
            // TODO: throw exception
            assert (values.length == columnFormats.length);

            for (int i = 0; i < columnFormats.length; i++) {
                output.printf(columnFormats[i], values[i]);
            }
            output.println();
        }

        private String makeFormatSpecifier(final int width, final String type) {
            return String.format("%%%d%s", width, type);
        }
    }
    
    /** Prevent instantiation. */
    private TextResultsWriter() {}
    
    /** Create CSV results writer.
     *
     * @param output Where to write the CSV stream.
     * @return CSV results writer.
     */
    public static ResultsWriter csv(final PrintStream output) {
        return new CsvOutput(output);
    }
    
    /** Create tabular results writer.
     *
     * @param output Where to write the table stream.
     * @return Tabular results writer.
     */
    public static ResultsWriter tabular(final PrintStream output) {
        return new TabularOutput(output);
    }
}
