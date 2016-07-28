package de.badtobi.jnichessengines;

import de.badtobi.jnichessengines.internal.OsHelper;
import de.badtobi.jnichessengines.internal.UciExternalEngine;

/**
 * Created by b4dt0bi on 13.07.16.
 */
public class Crafty extends UciExternalEngine {
    private static final String ENGINE_NAME = "crafty";

    public Crafty() {
        super(ENGINE_NAME, OsHelper.getExecutable(ENGINE_NAME));
    }
}
