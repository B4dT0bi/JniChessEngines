package de.badtobi.jnichessengines;

/**
 * Created by b4dt0bi on 12.07.16.
 */
public interface EngineLogger {
    /**
     * Will be called when a message has been received from the engine.
     * @param message
     */
    void messageFromEngine(String message);
    void messageToEngine(String message);
    void log(String tag, String message);
    void log(String tag, String message, Throwable throwable);
    void handleException(Throwable throwable);
}
