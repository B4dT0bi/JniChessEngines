package de.badtobi.jnichessengines.internal;

import de.badtobi.jnichessengines.ChessEngineException;
import de.badtobi.jnichessengines.ChessEngineListener;
import de.badtobi.jnichessengines.EngineLogger;
import de.badtobi.jnichessengines.Status;
import de.badtobi.jnichessengines.options.AbstractOption;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by b4dt0bi on 13.07.16.
 */
public class UciExternalEngine extends ExternalEngine implements Runnable {

    private String name;
    private String author;

    private boolean uciReady = false;
    private boolean running = false;
    private boolean initialized = false;

    private ChessEngineListener listener;

    private Map<String, AbstractOption> options = new HashMap<String, AbstractOption>();

    public UciExternalEngine(final String name, final String executable) {
        super(name, executable);
    }

    @Override
    public void init(final EngineLogger logger, final ChessEngineListener listener) {
        this.listener = listener;
        super.init(logger, listener);
        Thread t = new Thread(this);
        t.start();
        try {
            Thread.sleep(250);
        } catch (InterruptedException e) {

        }
        sendToUci("uci"); // activate UniversalChessInterface
    }

    @Override
    public Collection<AbstractOption> getOptions() {
        return options.values();
    }

    @Override
    public AbstractOption getOption(final String id) {
        return options.get(id);
    }

    @Override
    public void setOption(final AbstractOption option) {
        sendToUci(option.toUciString());
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public String getAuthor() {
        return author;
    }

    @Override
    public void newGame() {
        waitForInit();
        sendToUci("ucinewgame");
        isReady();
    }

    @Override
    public void position() {
        sendToUci("position startpos");
    }

    @Override
    public void position(final Collection<String> moves) {
        sendToUci("position startpos" + (moves != null && !moves.isEmpty() ? " moves " + toSpaceSeperated(moves) : ""));
    }

    @Override
    public void position(String fen, Collection<String> moves) {
        sendToUci("position fen " + fen + (moves != null && !moves.isEmpty() ? " moves " + toSpaceSeperated(moves) : ""));
    }

    @Override
    public void position(String fen) {
        position(fen, null);
    }

    private String toSpaceSeperated(Collection<String> strings) {
        String result = "";
        for (String s : strings) {
            result += s + " ";
        }
        return result.trim();
    }

    @Override
    public void go() {
        sendToUci("go");
    }

    @Override
    public void go(Collection<String> searchMoves, Boolean ponder, Long wTime, Long bTime, Long wInc, Long bInc, Integer movesToGo, Integer depth, Integer nodes, Integer mate, Long moveTime, Boolean infinite) {
        StringBuilder sb = new StringBuilder();
        sb.append("go");
        if (searchMoves != null && !searchMoves.isEmpty()) {
            sb.append(" searchmoves ").append(toSpaceSeperated(searchMoves).trim());
        }
        if (ponder != null && ponder) {
            sb.append(" ponder");
        }
        if (wTime != null && wTime > 0) {
            sb.append(" wtime ").append(wTime);
        }
        if (bTime != null && bTime > 0) {
            sb.append(" btime ").append(bTime);
        }
        if (wInc != null && wInc > 0) {
            sb.append(" winc ").append(wInc);
        }
        if (bInc != null && bInc > 0) {
            sb.append(" binc ").append(bInc);
        }
        if (movesToGo != null && movesToGo > 0) {
            sb.append(" movestogo ").append(movesToGo);
        }
        if (depth != null && depth > 0) {
            sb.append(" depth ").append(depth);
        }
        if (nodes != null && nodes > 0) {
            sb.append(" nodes ").append(nodes);
        }
        if (mate != null && mate > 0) {
            sb.append(" mate ").append(mate);
        }
        if (moveTime != null && moveTime > 0) {
            sb.append(" movetime ").append(moveTime);
        }
        if (infinite != null && infinite) {
            sb.append(" infinite");
        }
        sendToUci(sb.toString());
    }

    @Override
    public void setDebugMode(final boolean value) {
        sendToUci("debug " + (value ? "on" : "off"));
    }

    @Override
    public boolean isReady() {
        if (uciReady) {
            if (listener != null) listener.ready();
            return uciReady;
        }
        waitForInit();
        sendToUci("isready");
        return uciReady;
    }

    private void waitForInit() {
        while (!initialized) {
            try {
                Thread.sleep(50);
            } catch (InterruptedException e) {

            }
        }
    }

    @Override
    public void ponderHit() {
        sendToUci("ponderhit");
    }

    @Override
    public void stop() {
        sendToUci("stop");
    }

    @Override
    public void quit() {
        sendToUci("quit");
        super.quit();
    }

    @Override
    public void registerListener(final ChessEngineListener listener) {
        this.listener = listener;
    }

    private void sendToUci(final String message) {
        uciReady = false;
        sendToEngine(message);
    }


    @Override
    public void run() {
        running = true;

        // monitor output from uci
        while (running) {
            String line = readLineFromEngine(1500);
            if (line != null) {
                String tokens[] = line.split("\\s");
                int index = 0;
                if ("readyok".equals(tokens[index])) {
                    uciReady = true;
                    if (listener != null) listener.ready();
                } else if ("uciok".equals(tokens[index])) {
                    initialized = true;
                    isReady();
                    if (listener != null) listener.setupComplete();
                } else if ("id".equals(tokens[index])) {
                    index++;
                    if ("name".equals(tokens[index])) {
                        name = line.substring(line.indexOf(" ", line.indexOf(" ") + 1)).trim();
                    } else if ("author".equals(tokens[index])) {
                        author = line.substring(line.indexOf(" ", line.indexOf(" ") + 1)).trim();
                    }
                } else if ("bestmove".equals(tokens[index])) {
                    String bestMove = tokens[index + 1];
                    String ponder = null;
                    if (tokens.length > 2 && "ponder".equals(tokens[index + 2])) {
                        ponder = tokens[index + 3];
                    }
                    if (listener != null) {
                        listener.bestmove(bestMove, ponder);
                    }
                } else if ("info".equals(tokens[index])) {
                    if (listener != null) listener.info(line);
                } else if ("copyprotection".equals(tokens[index])) {
                    if (listener != null)
                        listener.copyprotection(Status.valueOf(tokens[index + 1]));
                } else if ("registration".equals(tokens[index])) {
                    if (listener != null) listener.registration(Status.valueOf(tokens[index + 1]));
                } else if ("option".equals(tokens[index])) {
                    try {
                        AbstractOption option = AbstractOption.getOptionByString(line);
                        options.put(option.getId(), option);
                    } catch (ChessEngineException e) {
                        getLogger().handleException(e);
                    }
                }
            }
        }
    }
}
