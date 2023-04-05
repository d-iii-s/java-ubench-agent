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

import java.util.NoSuchElementException;

/** Mapping between Java threads and their native ids. */
public final class NativeThreads {
    /** Invalid thread ID marker. */
    private static final long INVALID_THREAD_ID = -1;
    
    /** Prevent instantiation. */
    private NativeThreads() {}

    /** Retrieve the native ID of given Java thread.
     *
     * @param thread Java thread.
     * @return Native id of the given thread.
     * @throws java.util.NoSuchElementException when the thread is not known.
     */
    public static long getNativeId(final Thread thread) {
        long res = getNativeId(thread.getId());
        if (res == INVALID_THREAD_ID) {
            String msg = String.format("Thread %s is not known to the C agent.", thread);
            throw new NoSuchElementException(msg);
        }
        return res;
    }

    /** Retrieve the native ID of given Java thread id.
     *
     * @param javaThreadId Java thread i.d
     * @return Native id of the given thread or INVALID_THREAD_ID when the thread is not known.
     */
    private static native long getNativeId(long javaThreadId);

    /** Insert a mapping between Java thread and its native id.
     *
     * @param thread Java thread.
     * @param nativeId Native id of the thread.
     * @return Whether registration was successful.
     */
    public static boolean registerJavaThread(final Thread thread, final long nativeId) {
        return registerJavaThread(thread.getId(), nativeId);
    }

    /** Insert a mapping between Java thread and its native id.
     *
     * @param javaThreadId Java thread id.
     * @param nativeId Native id of the thread.
     * @return Whether registration was successful.
     */
    private static native boolean registerJavaThread(long javaThreadId, long nativeId);
}
