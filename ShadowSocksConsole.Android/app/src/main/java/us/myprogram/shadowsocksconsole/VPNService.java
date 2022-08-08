package us.myprogram.shadowsocksconsole;

import android.content.Intent;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.net.VpnService;
import android.os.ParcelFileDescriptor;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.channels.DatagramChannel;

public class VPNService extends VpnService {
    private Thread mThread;
    private ParcelFileDescriptor mInterface;
    //a. Configure a builder for the interface.
    Builder builder = new Builder();


    // Services interface
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // Start a new session by creating a new thread.
        mThread = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (ShadowSocksController.isNull()) {
                        ShadowSocksController.Create(getApplicationInfo().nativeLibraryDir, getApplication().getFilesDir().getAbsolutePath());
                    }
                    if (!ShadowSocksController.Get().Started())
                        ShadowSocksController.Get().Start();
                    {
                        File path = new File(getApplication().getFilesDir().getAbsolutePath(), "sock_path").getAbsoluteFile();
                        path.delete();
                    }
                    boolean m_inited = false;
                    boolean sock_connected = false;
                    LocalSocket s;
                    while (true) {
                        //get packet with in
                        //put packet to tunnel
                        //get packet form tunnel
                        //return packet with out
                        //sleep is a must
                        Thread.sleep(100);
                        if (!m_inited) {
                            if (ShadowSocksController.Get().GetT2SParams() == null) continue;

                            if (ShadowSocksController.Get().GetT2SParams().wait_service == false)
                                continue;
                            ShadowSocksController.Get().appendLog("Try init VPN\n");

                            //a. Configure the TUN and get the interface.
                            mInterface = builder.setSession("ShadowSocksConsole")
                                    .addAddress(ShadowSocksController.Get().GetT2SParams().ip, 24)
                                    .addDisallowedApplication("us.myprogram.shadowsocksconsole")
                                    .addDnsServer(ShadowSocksController.Get().GetT2SParams().gw)
                                    .addRoute("0.0.0.0", 0).establish();
                            if (mInterface == null) {
                                ShadowSocksController.Get().appendLog("Fail to start VPN mode\n");
                                continue;
                            }

                            ShadowSocksController.Get().appendLog("VPN Started\n");
                            ShadowSocksController.Get().GetT2SParams().inited = true;
                            m_inited = true;
                            sock_connected = false;
                        } else {
                            File path = new File(getApplication().getFilesDir().getAbsolutePath(), "sock_path").getAbsoluteFile();
                            if (!path.isFile() && sock_connected) {
                                m_inited = false;
                                try {
                                    if (mInterface != null) {
                                        mInterface.close();
                                        mInterface = null;
                                    }
                                } catch (Exception e) {

                                }
                                ShadowSocksController.Get().ClearT2SParams();
                                continue;
                            }
                            if (sock_connected)
                                continue;
                            try {
                                s = new LocalSocket();
                                s.connect(new LocalSocketAddress(path.toString(), LocalSocketAddress.Namespace.FILESYSTEM));
                                ShadowSocksController.Get().appendLog("!!!!!!!!!!!!!!Socket connected\n");
                                FileDescriptor[] d = new FileDescriptor[1];
                                d[0] = mInterface.getFileDescriptor();
                                s.setFileDescriptorsForSend(d);
                                s.getOutputStream().write(42);
                                ShadowSocksController.Get().appendLog("!!!!!!!!!!!!!!!!!EndLoop\n");
                                sock_connected = true;
                            } catch (Exception e) {}
                        }


                    }

                } catch (Exception e) {
                    StringWriter wr = new StringWriter();
                    PrintWriter ps = new PrintWriter(wr);
                    e.printStackTrace(ps);
                    ShadowSocksController.Get().appendLog("VPN Start error" + e.toString());
                    ShadowSocksController.Get().appendLog(wr.toString());
                    e.printStackTrace();
                } finally {
                    try {
                        if (mInterface != null) {
                            mInterface.close();
                            mInterface = null;
                        }
                    } catch (Exception e) {

                    }
                }
                ShadowSocksController.Get().appendLog("!!!!!!!!!!!!!!!!!ThreadFINISHED\n");
            }

        }, "ShadowSocksConsole");

        //start the service
        mThread.start();
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        // TODO Auto-generated method stub
        if (mThread != null) {
            mThread.interrupt();
        }
        super.onDestroy();
    }
}
