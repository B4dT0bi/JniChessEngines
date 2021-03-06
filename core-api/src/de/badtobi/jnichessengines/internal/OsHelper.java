package de.badtobi.jnichessengines.internal;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;

import de.badtobi.jnichessengines.ChessEngineException;

/**
 * Created by b4dt0bi on 13.07.16.
 */
public class OsHelper {

    public static String getExecutable(String name) {

        if (isAndroid()) return getAndroidExecutable(name);

        return getExecutable(name + ".elf", name + "64.elf", // linux
                name + ".exe", name + "64.exe", // windows
                name + ".mach", name + "64.mach"); // mac
    }

    private static AndroidOsHelperInterface androidOsHelperInterface;

    public static void setAndroidOsHelperInterface(AndroidOsHelperInterface iAndroidOsHelperInterface) {
        androidOsHelperInterface = iAndroidOsHelperInterface;
    }

    private static AndroidOsHelperInterface getAndroidOsHelperInterface() {
        if (androidOsHelperInterface == null) {
            throw new ChessEngineException("Cannot get Android Helper class. Did you add android-core as dependency? And instanciated it?");
        }
        return androidOsHelperInterface;
    }

    private static String getAndroidExecutable(String name) {
        AndroidOsHelperInterface aohi = getAndroidOsHelperInterface();
        for (String supportedAbi : aohi.getABIs()) {
            if (OsHelper.class.getClassLoader().getResource(supportedAbi + "/" + name) != null) {
                return supportedAbi + "/" + name;
            }
        }
        throw new ChessEngineException("Could not find an ABI which matches the supported ones (" + Arrays.toString(aohi.getABIs()) + ") for engine : " + name);
    }

    public static String getExecutable(String linux32, String linux64, String win32, String win64, String mac32, String mac64) {
        if (isWindows()) {
            return is64bit() ? win64 : win32;
        } else if (isUnix()) {
            return is64bit() ? linux64 : linux32;
        } else if (isMac()) {
            return is64bit() ? mac64 : mac32;
        }
        return null; // TODO : exception?
    }

    public static boolean is64bit() {
        return System.getProperty("os.arch").contains("64");
    }

    public static boolean isWindows() {
        return (System.getProperty("os.name").toLowerCase().contains("win"));
    }

    public static boolean isMac() {
        return (System.getProperty("os.name").toLowerCase().contains("mac"));
    }

    public static boolean isUnix() {
        return (System.getProperty("os.name").toLowerCase().contains("nix") || System.getProperty("os.name").toLowerCase().contains("nux") || System.getProperty("os.name").toLowerCase().indexOf("aix") > 0);
    }

    public static boolean isAndroid() {
        return System.getProperty("java.vendor").contains("Android");
    }

    public static void setExecuteableFlag(File file) {
        if (isAndroid()) {
            if (getAndroidOsHelperInterface().setExecuteableFlag(file.getAbsolutePath())) {
                return;
            }
        }
        if (!file.setExecutable(true)) {
            if (isUnix() || isMac()) {
                try {
                    Runtime.getRuntime().exec("chmod 744 " + file.getAbsolutePath());
                } catch (IOException e) {

                }
            }
        }
    }

    public static String getPathToWrite() {
        return isAndroid() ? getAndroidOsHelperInterface().getPathToWrite() : "";
    }
}
