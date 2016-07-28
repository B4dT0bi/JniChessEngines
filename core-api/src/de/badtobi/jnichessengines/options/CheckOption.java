package de.badtobi.jnichessengines.options;

/**
 * Created by b4dt0bi on 12.07.16.
 */
public class CheckOption extends AbstractOption {

    public boolean getValue() {
        if (value == null) return "true".equals(defaultValue);
        return "true".equals(value);
    }

    public void setValue(boolean bValue) {
        value = bValue ? "true" : "false";
    }

    public void toggle() {
        setValue(!getValue());
    }

    public void reset() {
        value = null;
    }

}
