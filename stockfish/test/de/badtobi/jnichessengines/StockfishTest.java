package de.badtobi.jnichessengines;

import org.junit.Assert;
import org.junit.Test;

/**
 * Created by b4dt0bi on 13.07.16.
 */
public class StockfishTest implements ChessEngineListener, EngineLogger {
    boolean ready = false;
    boolean gotMove = false;

    @Test
    public void testEngine() {
        Stockfish stockfish = new Stockfish();
        stockfish.init(this, this);
        stockfish.isReady();
        while (!ready) {
            try {
                Thread.sleep(50);
            } catch (InterruptedException e) {

            }
        }
        Assert.assertEquals("Stockfish 130716", stockfish.getName());
        Assert.assertEquals("T. Romstad, M. Costalba, J. Kiiski, G. Linscott", stockfish.getAuthor());

        stockfish.newGame();
        stockfish.position();
        stockfish.go();

        while (!gotMove) {
            try {
                Thread.sleep(50);
            } catch (InterruptedException e) {

            }
        }

        stockfish.quit();
    }

    @Override
    public void setupComplete() {
        System.out.println("setupComplete");
    }

    @Override
    public void ready() {
        System.out.println("ready");
        ready = true;
    }

    @Override
    public void bestmove(String move, String ponder) {
        System.out.println("bestmove : move " + move + " ponder : " + ponder);
        gotMove = true;
    }

    @Override
    public void copyprotection(Status status) {
        System.out.println("copyprotection status : " + status);
    }

    @Override
    public void registration(Status status) {
        System.out.println("registration status : " + status);
    }

    @Override
    public void info(String message) {
        System.out.println("info : " + message);
    }

    @Override
    public void messageFromEngine(String message) {
        System.out.println("msg from engine : " + message);
    }

    @Override
    public void messageToEngine(String message) {
        System.out.println("msg to engine : " + message);
    }

    @Override
    public void log(String tag, String message) {
        System.out.println("log tag : " + tag + " message : " + message);
    }

    @Override
    public void log(String tag, String message, Throwable throwable) {
        System.out.println("log tag : " + tag + " message : " + message);
        throwable.printStackTrace();
    }

    @Override
    public void handleException(Throwable throwable) {
        System.out.println("handleException");
        throwable.printStackTrace();
    }
}
