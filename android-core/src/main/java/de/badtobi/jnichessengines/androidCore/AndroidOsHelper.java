package de.badtobi.jnichessengines.androidCore;

import android.os.Build;

import de.badtobi.jnichessengines.internal.AndroidOsHelperInterface;

/**
 * Created by b4dt0bi on 14.07.16.
 */
public class AndroidOsHelper implements AndroidOsHelperInterface {

    public String[] getABIs() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            return Build.SUPPORTED_ABIS;
        } else {
            return new String[] {Build.CPU_ABI};
        }
    }


}
