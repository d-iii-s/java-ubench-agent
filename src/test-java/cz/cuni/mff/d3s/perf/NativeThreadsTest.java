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

import org.junit.*;

public class NativeThreadsTest {

    private static class SpinningWorker implements Runnable {
        public volatile boolean terminate = false;
        public volatile boolean started = false;
        @Override
        public void run() {
            started = true;
            while (!terminate) {}
        }
    }

    private SpinningWorker worker;
    private Thread thread;

    @Before
    public void setup() {
        // Make sure the agent library is loaded before creating new threads.
        UbenchAgent.load();
        
        worker = new SpinningWorker();
        thread = new Thread(worker, "SpinningWorker");
        
        thread.start();
        while (!worker.started) {}
    }

    @After
    public void teardown() {
        if (thread.isAlive()) {
            worker.terminate = true;
            try {
                thread.join();
            } catch (InterruptedException e) {}
        }
    }

    @Test
    public void newThreadsAreAutomaticallyRegistered() {
        long id = NativeThreads.getNativeId(thread);

        // Getting here means the previous call has not thrown any
        // exception, thus the thread was registered.
        Assert.assertTrue(true);
    }

    @Test
    public void impossibleToRegisterThreadTwice() {
        // If the agent was actually loaded through 'UBenchAgent.load()', the
        // main thread is already running and its start will have been missed
        // by the agent and will not be automatically registered.
        //
        // Try re-registering the worker thread instead.
        Assert.assertFalse(NativeThreads.registerJavaThread(thread, 0));
    }
}
