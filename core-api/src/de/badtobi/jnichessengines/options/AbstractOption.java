package de.badtobi.jnichessengines.options;

import java.util.ArrayList;
import java.util.List;

import de.badtobi.jnichessengines.ChessEngineException;

/**
 * Created by b4dt0bi on 12.07.16.
 */
public abstract class AbstractOption {
    protected String id;
    protected String type;
    protected String value;
    protected String defaultValue;
    protected String minValue;
    protected String maxValue;
    private OptionApplyer optionApplyer = null;
    protected List<String> vars = new ArrayList<String>();

    public static AbstractOption getOptionByString(String str) {
        String tokens[] = str.split(" ");
        String type = getType(tokens);
        AbstractOption result = null;
        if ("check".equals(type)) {
            result = new CheckOption();
        } else if ("spin".equals(type)) {
            result = new SpinOption();
        } else if ("string".equals(type)) {
            result = new StringOption();
        } else if ("button".equals(type)) {
            result = new ButtonOption();
        } else if ("combo".equals(type)) {
            result = new ComboOption();
        } else {
            throw new ChessEngineException("unknown option : " + str);
        }

        result.type = type;
        result.id = tokens[2];
        int i = 3;
        while (!"type".equals(tokens[i])) {
            result.id += " " + tokens[i];
            i++;
        }
        for (; i < tokens.length; i++) {
            if ("default".equals(tokens[i])) {
                if (i + 1 >= tokens.length) {
                    result.defaultValue = "string".equals(type) ? "<empty>" : "";
                } else {
                    result.defaultValue = tokens[i + 1];
                    i++;
                }
            } else if ("min".equals(tokens[i])) {
                result.minValue = tokens[i + 1];
                i++;
            } else if ("max".equals(tokens[i])) {
                result.maxValue = tokens[i + 1];
                i++;
            } else if ("var".equals(tokens[i])) {
                result.vars.add(tokens[i + 1]);
                i++;
            }
        }
        return result;
    }

    private static String getType(String[] token) {
        for (int i = 0; i < token.length; i++) {
            if ("type".equals(token[i])) return token[i + 1];
        }
        return "";
    }

    public String toUciString() {
        return value != null ? "setoption name " + id + " value " + value : null;
    }

    public String getId() {
        return id;
    }

    public void apply() {
        if (optionApplyer != null) optionApplyer.applyOption(this);
    }

    public void registerOptionApplyer(final OptionApplyer optionApplyer) {
        this.optionApplyer = optionApplyer;
    }
}
