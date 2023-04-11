package cz.cuni.mff.d3s.perf;

/**
 * Agent library loader. Allows loading the 'ubench-agent' library explicitly 
 * using {@link System#loadLibrary()} (if not loaded as native agent).
 */
final class UbenchAgent {
    
    /** Prevent instantiation. */
    private UbenchAgent() {}
    
    /** Loads the agent library (if not already loaded). */
    static void load() {
        if (!isLoaded()) {
            System.loadLibrary("ubench-agent");
        }
    }
    
    /** Determines if the agent library is loaded. 
     * 
     * @return {@code true} if the native library is loaded, {@code false} otherwise.
     */
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
    
    /** Utility method to determine if the agent library is loaded. 
     * 
     * @return {@code true} if the native library is loaded.
     * @throws UnsatisfiedLinkError if the native library is not loaded.
     */
    private static native boolean nativeIsLoaded();
    
}
