package de.badtobi.jnichessengines.options;

/**
 * Created by b4dt0bi on 12.07.16.
 */
public class ComboOption extends AbstractOption {

    public String getValue() {
        if (value == null) return defaultValue;
        return value;
    }

    public void setValue(String sValue) {
        if (vars != null && !vars.isEmpty()) {
            if (!vars.contains(sValue)) {
                throw new RuntimeException("Value not in the allowed ranges");
            }
        }
    }

    public String [] getAllowedValues() {
        if (vars != null && !vars.isEmpty()) {
            return vars.toArray(new String[vars.size()]);
        }
        return null;
    }

    public void reset() {
        value = null;
    }

}
