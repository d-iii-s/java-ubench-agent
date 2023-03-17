package cz.cuni.mff.d3s.perf;

final class UbenchAgent {
    volatile static boolean alreadyLoaded = false;

    static void load() {
        if (!alreadyLoaded) {
            System.loadLibrary("ubench-agent");
        }
    }
}
