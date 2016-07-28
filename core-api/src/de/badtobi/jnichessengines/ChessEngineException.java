package de.badtobi.jnichessengines;

/**
 * Created by b4dt0bi on 13.07.16.
 */
public class ChessEngineException extends RuntimeException {
    public ChessEngineException() {
    }

    public ChessEngineException(String s) {
        super(s);
    }

    public ChessEngineException(String s, Throwable throwable) {
        super(s, throwable);
    }

    public ChessEngineException(Throwable throwable) {
        super(throwable);
    }

    public ChessEngineException(String s, Throwable throwable, boolean b, boolean b1) {
        super(s, throwable, b, b1);
    }
}
