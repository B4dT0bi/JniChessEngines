package de.badtobi.jnichessengines.internal;

import de.badtobi.jnichessengines.EngineLogger;

/**
 * Created by b4dt0bi on 13.07.16.
 */
public class NoopLogger implements EngineLogger {
    @Override
    public void messageFromEngine(String message) {

    }

    @Override
    public void messageToEngine(String message) {

    }

    @Override
    public void log(String tag, String message) {

    }

    @Override
    public void log(String tag, String message, Throwable throwable) {
        throwable.printStackTrace();
    }

    @Override
    public void handleException(Throwable throwable) {
        throwable.printStackTrace();
    }
}
