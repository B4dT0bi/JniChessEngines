package de.badtobi.jnichessengines.androidCore;

import android.content.Context;
import android.os.Build;
import android.os.Environment;

import java.io.File;

import de.badtobi.jnichessengines.internal.AndroidOsHelperInterface;

/**
 * Created by b4dt0bi on 14.07.16.
 */
public class AndroidOsHelper implements AndroidOsHelperInterface {
    private Context context;

    public AndroidOsHelper(Context context) {
        this.context = context;
    }

    static {
        System.loadLibrary("nHelper");
    }

    public String[] getABIs() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            return addNoPIEIfNeeded(Build.SUPPORTED_ABIS);
        } else {
            return addNoPIEIfNeeded(new String[]{Build.CPU_ABI});
        }
    }

    private String[] addNoPIEIfNeeded(String [] abis) {
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            String [] result = new String[abis.length];
            for (int i = 0; i < result.length; i++) {
                result[i] = abis[i] + (abis[i].contains("64") ? "" : "-noPIE");
            }
            return result;
        } else {
            return abis;
        }
    }
    @Override
    public String getPathToWrite() {
        return context.getFilesDir().getAbsolutePath() + "/";
    }

    @Override
    public boolean setExecuteableFlag(String file) {
        return ensureExecutable(file);
    }

    static native boolean ensureExecutable(String file);
}
