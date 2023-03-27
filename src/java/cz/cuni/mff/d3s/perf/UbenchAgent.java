package cz.cuni.mff.d3s.perf;

final class UbenchAgent {
    
    static void load() {
        // Load the agent library if not already loaded.
        if (!isLoaded()) {
            System.loadLibrary("ubench-agent");
        }
    }
    
    private static boolean isLoaded() {
        //
        // Try calling a harmless native method in the agent. If the call
        // succeeds, it means that the library has been already loaded and
        // its native methods were bound. If it fails, the library needs to
        // be loaded explicitly.
        //
        try {
            return nativeIsLoaded();
        } catch (final UnsatisfiedLinkError e) {
            return false;
        }
    }
    
    private native static boolean nativeIsLoaded();
    
}
