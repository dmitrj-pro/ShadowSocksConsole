package us.myprogram.shadowsocksconsole;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.util.LinkedList;
import java.util.List;

public class ShadowSocksController{
    public class Tun2SocksParams{
        public boolean inited = false;
        public boolean wait_service = false;
        public String ip = "";
        public String gw = "";
        public List<String> ignore_ip = new LinkedList<String>();
    }

    private String nativeLibraryDir;
    private String writableDir;
    private String cacheDir;
    private ShadowSocksConsoleProcess process = null;
    private Tun2SocksParams t2sparams = null;

    private StringBuffer log = null;

    private static ShadowSocksController inner = null;

    public static ShadowSocksController Get() { return inner; }
    public static boolean isNull() { return inner == null;}
    public static void Create(String nativeLibraryDir, String getFilesDir, String cacheDir) {
        inner = new ShadowSocksController(nativeLibraryDir, getFilesDir, cacheDir);
    }
    public Tun2SocksParams GetT2SParams() { return t2sparams; }
    public void ClearT2SParams() { t2sparams = new Tun2SocksParams(); }

    private ShadowSocksController(String nativeLibraryDir, String getFilesDir, String cacheDir) {
        this.nativeLibraryDir = nativeLibraryDir;
        this.writableDir = getFilesDir;
        this.cacheDir = cacheDir;
    }

    public String CacheDir() {
        return cacheDir;
    }

    public boolean Started() {
        if (process == null)
            return false;
        return process.Started();
    }
    public boolean isFullStarted() {
        if (!Started()) return false;
        return log.toString().contains("web server");
    }
    public void appendLog(String str) {
        if (log == null) return;
        log.append(str);
    }

    public String getLogging() {
        if (log == null)
            return "";
        return log.toString();
    }

    public void Start() {
        process = new ShadowSocksConsoleProcess(nativeLibraryDir, writableDir, cacheDir);
        log = new StringBuffer();
        t2sparams = new Tun2SocksParams();

        {
            File directory = new File(nativeLibraryDir);
            File[] files = directory.listFiles();
            log.append("Files in " + directory.getAbsolutePath() + ":\n");
            if (files == null) {
                log.append("Fail to get files in directory\n");
            } else {
                for (int i = 0; i < files.length; i++)
                    log.append(files[i].getName() + "\n");
            }
        }
        {
            File directory = new File(nativeLibraryDir + "../");
            File[] files = directory.listFiles();
            log.append("Files in " + directory.getAbsolutePath() + ":\n");
            if (files == null) {
                log.append("Fail to get files in directory\n");
            } else {
                for (int i = 0; i < files.length; i++)
                    log.append(files[i].getName() + "\n");
            }
        }



        process.start();
    }
    public void Stop() {
        if (process == null)
            return;
        process.Stop();
    }
    public void KILL() {
        if (process == null)
            return;
        process.KILL();
    }
}
