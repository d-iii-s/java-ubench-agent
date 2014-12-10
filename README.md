Microbenchmarking Agent for Java
================================

A JVMTI agent to be used with microbenchmarks. Basic features:

- Reports major JVM events such as GC and JIT runs.
- Can collect performance counters through JNI.
- Can collect accurate time through JNI.


Usage
-----
```java
private static final int LOOPS = 10;
private static final String[] EVENTS = {
  "SYS_WALLCLOCK",
  "JVM_COMPILATIONS",
  /* Only on Linux with PAPI installed */
  "PAPI_L1_DCM"
};
	
public static void myBenchmark() {
  /* We would have LOOPS measurements and we
     want to record these EVENTS. */
  Benchmark.init(LOOPS, EVENTS);
  
  for (int i = 0; i < LOOPS; i++) {
    /* Start the measurement. */
    Benchmark.start();
    
    /* Your code that ought to be measured. */
    
    /* Stop the measurement. */
    Benchmark.stop();
  }
  
  /* Get the results (available as Iterable<long[]>). */
  BenchmarkResults results = Benchmark.getResults();
  
  /* Either print them in CSV (to be later processed)... */
  BenchmarkResultsPrinter.toCsv(results, System.out);
  
  /* ... or as a space-padded table. */
  BenchmarkResultsPrinter.table(results, System.out);
}
```

Compilation
-----------
You will need recent version of Ant and GCC. Then simple
```
ant && ant test test-junit
```
shall compile the agent and run the built-in tests
(you might need to set `JAVA_HOME` if you have JDK in non-standard location).
