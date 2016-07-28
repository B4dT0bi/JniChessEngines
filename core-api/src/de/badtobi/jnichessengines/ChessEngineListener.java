package de.badtobi.jnichessengines;

/**
 * Created by b4dt0bi on 12.07.16.
 */
public interface ChessEngineListener {
    void setupComplete();
    void ready();
    void bestmove(String move, String ponder);
    void copyprotection(Status status);
    void registration(Status status);
    void info(String message);
}
