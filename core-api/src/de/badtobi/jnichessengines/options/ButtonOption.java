package de.badtobi.jnichessengines.options;

/**
 * Created by b4dt0bi on 12.07.16.
 */
public class ButtonOption extends AbstractOption {

    public String toUciString() {
        return "setoption name " + id;
    }
}
