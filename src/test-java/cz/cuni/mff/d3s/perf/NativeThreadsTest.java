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
        @Override
        public void run() {
            while (!terminate) {}
        }
    }
    
    private SpinningWorker worker;
    private Thread thread;
    
    @Before
    public void setup() {
        worker = new SpinningWorker();
        thread = new Thread(worker);
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
        thread.start();
        long id = NativeThreads.getNativeId(thread);
        
        // Getting here means the previous call has not thrown any
        // exception, thus the thread was registered.
        Assert.assertTrue(true);
    }
    
    @Test
    public void impossibleToRegisterThreadTwice() {
        Assert.assertFalse(NativeThreads.registerJavaThread(Thread.currentThread(), 0));
    }
}
