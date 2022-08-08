package us.myprogram.shadowsocksconsole;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.util.LinkedList;
import java.util.List;

public class ShadowSocksConsoleProcess extends Thread {

    private Process SSC = null;
    private String nativeLibraryDir;
    private String writableDir;
    private boolean running = false;
    private boolean needClose = false;
    private boolean killSignal = false;


    public ShadowSocksConsoleProcess(String nativeLibraryDir, String getFilesDir) {
        this.nativeLibraryDir = nativeLibraryDir;
        this.writableDir = getFilesDir;
    }

    public boolean Started() {
        return running;
    }

    public void Start() {
        needClose = false;
        String bin_name="libShadowSocksConsole.so";
        File bin = new File(nativeLibraryDir, bin_name);
        if (!bin.isFile()) {
            ShadowSocksController.Get().appendLog("Binary file " + bin.getAbsolutePath() + " not found");
            return;
        }
        List<String> cmd = new LinkedList<String>();

        cmd.add(bin.getAbsolutePath());
        cmd.add("--set-main-dir");
        cmd.add(writableDir);
        cmd.add("--set-depend-dir");
        cmd.add(nativeLibraryDir);
        cmd.add("sexecute");

        ProcessBuilder builder = new ProcessBuilder();
        builder.command(cmd);
        ShadowSocksController.Get().appendLog("Execute command " + bin.getAbsolutePath() + "\n");

        try {
            SSC = builder.start();
            running = true;

            InputStreamReader cout = new InputStreamReader(SSC.getInputStream());
            BufferedReader reader = new BufferedReader(cout);

            OutputStreamWriter cin = new OutputStreamWriter(SSC.getOutputStream());
            String line = "";

            while (true) {
                try {
                    int r = SSC.exitValue();
                    break;
                }catch (IllegalThreadStateException e) {

                }

                if (needClose) {
                    try {
                        cin.write("EXIT\n");
                        cin.flush();
                        ShadowSocksController.Get().appendLog("Sended command exit\n");
                        needClose = false;
                    }catch (IOException e) {
                        ShadowSocksController.Get().appendLog("Catch exeption while send close to ShadowSocksConsole " + e.toString());
                        SSC.destroy();
                    }
                }
                if (killSignal) {
                    try {
                        cin.write("KILL\n");
                        cin.flush();
                        ShadowSocksController.Get().appendLog("Sended command KILL\n");
                        killSignal = false;
                    }catch (IOException e) {
                        ShadowSocksController.Get().appendLog("Catch exeption while send close to ShadowSocksConsole " + e.toString());
                        SSC.destroy();
                    }
                }
                if (ShadowSocksController.Get().GetT2SParams().inited && ShadowSocksController.Get().GetT2SParams().wait_service) {
                    cin.write("SERVICE_STARTED\n");
                    ShadowSocksController.Get().GetT2SParams().wait_service = false;
                    cin.flush();
                    ShadowSocksController.Get().appendLog("Sended command next T2S\n");
                }

                if (reader.ready()) {
                    line = reader.readLine();
                    if (line == null)
                        break;

                    String TUN_GW = "[TUN_GW:";
                    if (line.contains(TUN_GW)) {
                        int pos = line.indexOf(TUN_GW);
                        pos += TUN_GW.length();
                        int end = line.indexOf("]");
                        String gw = line.substring(pos, end);
                        ShadowSocksController.Get().GetT2SParams().gw = gw;
                        ShadowSocksController.Get().appendLog("GW = " + gw + "\n");
                    }

                    String TUN_ADDR = "[TUN_ADDR:";
                    if (line.contains(TUN_ADDR)) {
                        int pos = line.indexOf(TUN_ADDR);
                        pos += TUN_ADDR.length();
                        int end = line.indexOf("]");
                        String gw = line.substring(pos, end);
                        ShadowSocksController.Get().GetT2SParams().ip = gw;
                        ShadowSocksController.Get().appendLog("IP = " + gw + "\n");
                    }

                    String IGNORE_IP = "[IGNORE_IP:";
                    if (line.contains(IGNORE_IP)) {
                        int pos = line.indexOf(IGNORE_IP);
                        pos += IGNORE_IP.length();
                        int end = line.indexOf("]");
                        String gw = line.substring(pos, end);
                        ShadowSocksController.Get().GetT2SParams().ignore_ip.add(gw);
                        ShadowSocksController.Get().appendLog("IGNORE = " + gw + "\n");
                    }

                    String T2S_STARTED = "[T2S_STARTED]";
                    if (line.contains(T2S_STARTED)) {
                        ShadowSocksController.Get().appendLog("T2S Started\n");
                    }

                    String WAIT_SERVICE = "[WAIT_SERVICE]";
                    if (line.contains(WAIT_SERVICE)) {
                        ShadowSocksController.Get().GetT2SParams().wait_service = true;
                        ShadowSocksController.Get().appendLog("T2S Wait for start\n");
                    }


                    //System.out.println(line + "\n");
                    ShadowSocksController.Get().appendLog(line + "\n");
                }
                if (ShadowSocksController.Get().GetT2SParams().inited)
                    Thread.sleep(100);
                else
                    Thread.sleep(50);

            }
            SSC.waitFor();
            running = false;
            ShadowSocksController.Get().appendLog("ShadowSocksConsole finished with code: " + String.valueOf(SSC.exitValue()) + "\n");
        }catch (IOException e) {
            ShadowSocksController.Get().appendLog("Catch exeption " + e.toString() + "\n");
        } catch (InterruptedException e) {
            ShadowSocksController.Get().appendLog("Catch exeption " + e.toString() + "\n");
        }
        System.out.println(ShadowSocksController.Get().getLogging());
    }
    public void run() {
        Start();
    }
    public void KILL() {
        if (SSC == null)
            return;
        this.killSignal = true;
    }
    public void Stop() {
        if (SSC == null)
            return;
        this.needClose = true;
    }
}
