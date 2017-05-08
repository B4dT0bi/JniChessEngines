package de.badtobi.jnichessengines.internal;

import de.badtobi.jnichessengines.ChessEngine;
import de.badtobi.jnichessengines.ChessEngineException;
import de.badtobi.jnichessengines.ChessEngineListener;
import de.badtobi.jnichessengines.EngineLogger;

import java.io.*;

/**
 * Created by b4dt0bi on 13.07.16.
 */
public abstract class ExternalEngine implements ChessEngine {
    private EngineLogger logger = new NoopLogger();

    private String executable;
    private String name;

    protected OutputStream toEngine;
    protected InputStream fromEngine;

    private final LocalPipe inLines = new LocalPipe();

    protected boolean isRunning = false;
    private boolean startedOk = false;

    private Process engineProcess;
    private Thread exitThread;

    public ExternalEngine(final String name, final String executable) {
        this.name = name;
        this.executable = executable;
    }

    @Override
    public void init(EngineLogger logger, ChessEngineListener listener) {
        this.logger = logger;
        try {
            File engine = prepEngine();
            engineProcess = new ProcessBuilder().command(engine.getAbsolutePath()).start();
            toEngine = engineProcess.getOutputStream();
            fromEngine = engineProcess.getInputStream();

            // Start a thread to ignore stderr
            Thread stdErrThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    byte[] buffer = new byte[128];
                    while (true) {
                        Process ep = engineProcess;
                        if ((ep == null) || Thread.currentThread().isInterrupted())
                            return;
                        try {
                            int len = ep.getErrorStream().read(buffer, 0, 1);
                            if (len < 0)
                                break;
                        } catch (IOException e) {
                            return;
                        }
                    }
                }
            });
            stdErrThread.start();
            exitThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    try {
                        Process ep = engineProcess;
                        if (ep != null)
                            ep.waitFor();
                        isRunning = false;
                        if (startedOk)
                            getLogger().log("error", "failed to start engine");
                        else {
                            getLogger().log("error", "engine terminated");
                        }
                    } catch (InterruptedException e) {
                    }
                }
            });
            exitThread.start();

            // Start a thread to read stdin
            Thread stdInThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    Process ep = engineProcess;
                    if (ep == null)
                        return;
                    InputStream is = ep.getInputStream();
                    InputStreamReader isr = new InputStreamReader(is);
                    BufferedReader br = new BufferedReader(isr, 8192);
                    String line;
                    try {
                        boolean first = true;
                        while ((line = br.readLine()) != null) {
                            if ((ep == null) || Thread.currentThread().isInterrupted())
                                return;
                            synchronized (inLines) {
                                inLines.addLine(line);
                                if (first) {
                                    startedOk = true;
                                    isRunning = true;
                                    first = false;
                                }
                            }
                        }
                    } catch (IOException e) {
                    }
                    inLines.close();
                }
            });
            stdInThread.start();
        } catch (IOException e) {
            throw new ChessEngineException(e);
        }
    }

    @Override
    public void quit() {
        if (exitThread != null) {
            exitThread.interrupt();
        }
        if (engineProcess != null) {
            engineProcess.destroy();
            engineProcess = null;
        }
    }

    @Override
    public void setLogger(final EngineLogger logger) {
        this.logger = logger;
    }

    protected EngineLogger getLogger() {
        return logger;
    }

    private File prepEngine() throws FileNotFoundException {
        File engineFile = pathToEngine();
        if (engineFile.exists()) {
            OsHelper.setExecuteableFlag(engineFile);
            return engineFile; // TODO : check lastmod ?
        }
        InputStream engineStream = findResource(executable);
        if (engineStream == null) throw new ChessEngineException("executable ("+executable+") could not be found, check dependencies!");
        copyEngine(engineStream, engineFile);
        OsHelper.setExecuteableFlag(engineFile);
        return engineFile;
    }

    private File pathToEngine() {
        return new File(OsHelper.getPathToWrite() + name);
    }

    private InputStream findResource(String name) throws FileNotFoundException {
        if (new File(name).exists()) return new FileInputStream(name);
        // TODO : parse jars? https://stackoverflow.com/questions/5193786/how-to-use-classloader-getresources-correctly
        return getClass().getClassLoader().getResourceAsStream(name);
    }

    private void copyEngine(InputStream engineStream, File dst) {
        OutputStream outputStream = null;

        try {
            // write the inputStream to a FileOutputStream
            outputStream =
                    new FileOutputStream(dst);

            int read = 0;
            byte[] bytes = new byte[1024];

            while ((read = engineStream.read(bytes)) != -1) {
                outputStream.write(bytes, 0, read);
            }

        } catch (IOException e) {
            getLogger().handleException(e);
        } finally {
            if (engineStream != null) {
                try {
                    engineStream.close();
                } catch (IOException e) {
                    getLogger().handleException(e);
                }
            }
            if (outputStream != null) {
                try {
                    // outputStream.flush();
                    outputStream.close();
                } catch (IOException e) {
                    getLogger().handleException(e);
                }
            }
        }
    }

    protected void sendToEngine(final String message) {
        getLogger().messageToEngine(message);
        if (engineProcess != null) {
            try {
                engineProcess.getOutputStream().write((message + "\n").getBytes());
                engineProcess.getOutputStream().flush();
            } catch (IOException e) {
                getLogger().handleException(e);
            }
        } else {
            getLogger().log("sendToEngine", "engine is not ready to receive");
        }
    }

    protected String readLineFromEngine(int timeoutMillis) {
        String ret = inLines.readLine(timeoutMillis);
        if (ret == null)
            return null;
        startedOk = true;
        if (ret.length() > 0) {
            getLogger().messageFromEngine(ret);
        }
        return ret;
    }
}
