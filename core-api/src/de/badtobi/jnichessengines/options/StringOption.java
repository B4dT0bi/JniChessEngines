package de.badtobi.jnichessengines.options;

/**
 * Created by b4dt0bi on 12.07.16.
 */
public class StringOption extends AbstractOption {

    public String getValue() {
        if (value == null) return "<empty>".equals(defaultValue) ? "" : defaultValue;
        return "<empty>".equals(value) ? "" : value;
    }

    public void setValue(String sValue) {
        value = "".equals(sValue) ? "<empty>" : sValue;
    }

    public void reset() {
        value = null;
    }
}
