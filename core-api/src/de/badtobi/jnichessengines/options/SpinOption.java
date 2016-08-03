package de.badtobi.jnichessengines.options;

/**
 * Created by b4dt0bi on 12.07.16.
 */
public class SpinOption extends AbstractOption {

    public int getValue() {
        if (value == null || "".equals(value)) return Integer.parseInt(defaultValue);
        return Integer.parseInt(value);
    }

    public void setValue(int iValue) {
        value = "" + iValue;
    }

    public int getMin() {
        return minValue == null ? Integer.MIN_VALUE : Integer.parseInt(minValue);
    }

    public int getMax() {
        return maxValue == null ? Integer.MAX_VALUE : Integer.parseInt(maxValue);
    }

    public void reset() {
        value = null;
    }
}
