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
    	
		/* Here goes your code that ought to be measured. */
    
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

Demo
----
Package `cz.cuni.mff.d3s.perf.demo` contains a small demo: the `MeasureHashing`
class measures performance of a file hashing with Java built-in MD5.
Run it with
```
ant demo
```
to produce results that could look like this:
```
demo:
     [java]   SYS_WALLCLOCK JVM_COMPILATIO   PAPI_TOT_INS    PAPI_L1_DCM
     [java]         1516055              9        5539366         107004
     [java]         1082882              0        3994763          14362
     [java]          983808              1        3690908          13472
     [java]          258992              0        1551475           7049
     [java]           69000              1         468639           1405
     [java]           67542              0         470675           1296
     ...
     <truncated>
     ...
     [java]           67091              0         470675           1286
     [java]           68760              1         469812           1295
     [java] Hash is d41d8cd98f00b204e9800998ecf8427e.
```
