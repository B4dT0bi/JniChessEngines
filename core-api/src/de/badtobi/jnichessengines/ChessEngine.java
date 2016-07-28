package de.badtobi.jnichessengines;

import de.badtobi.jnichessengines.options.AbstractOption;

import java.util.Collection;
import java.util.List;

/**
 * Created by b4dt0bi on 12.07.16.
 */
public interface ChessEngine {
    /**
     * Get a list of all options from the Engine.
     *
     * @return
     */
    Collection<AbstractOption> getOptions();

    /**
     * Get the option identified by @param id.
     *
     * @param id
     * @return
     */
    AbstractOption getOption(String id);

    /**
     * Set the given option.
     *
     * @param option
     */
    void setOption(AbstractOption option);

    /**
     * Get the name of the Engine.
     *
     * @return
     */
    String getName();

    /**
     * Get the Author of the engine.
     *
     * @return
     */
    String getAuthor();

    /**
     * Initialize the Engine.
     */
    void init(EngineLogger logger, ChessEngineListener listener);

    /**
     * this is sent to the engine when the next search (started with "position" and "go") will be from
     * a different game. This can be a new game the engine should play or a new game it should analyse but
     * also the next position from a testsuite with positions only.
     * <p/>
     * As the engine's reaction to "newGame" can take some time the GUI should always send "isReady"
     * after "newGame" to wait for the engine to finish its operation.
     */
    void newGame();

    void position();

    void position(Collection<String> moves);

    void position(String fen);

    void position(String fen, Collection<String> moves);

    void go();
    void go(Collection<String> searchMoves, Boolean ponder, Long wTime, Long bTime, Long wInc, Long bInc, Integer movesToGo, Integer depth, Integer nodes, Integer mate, Long moveTime, Boolean infinite);

    void setDebugMode(boolean value);

    /**
     * this is used to synchronize the engine with the GUI. When the GUI has sent a command or
     * multiple commands that can take some time to complete,
     * this command can be used to wait for the engine to be ready again or
     * to ping the engine to find out if it is still alive.
     * E.g. this should be sent after setting the path to the tablebases as this can take some time.
     * This command is also required once before the engine is asked to do any search
     * to wait for the engine to finish initializing.
     * This command must always be answered with "readyok" and can be sent also when the engine is calculating
     * in which case the engine should also immediately answer with "readyok" without stopping the search.
     *
     * @return
     */
    boolean isReady();

    void ponderHit();

    void stop();

    void quit();

    void setLogger(EngineLogger logger);

    void registerListener(ChessEngineListener listener);
}
