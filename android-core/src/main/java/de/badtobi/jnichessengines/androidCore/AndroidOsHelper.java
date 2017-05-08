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
            return Build.SUPPORTED_ABIS;
        } else {
            return new String[]{Build.CPU_ABI};
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
