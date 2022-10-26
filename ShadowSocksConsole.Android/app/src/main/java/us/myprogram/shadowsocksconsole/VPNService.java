package us.myprogram.shadowsocksconsole;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.net.VpnService;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.support.v4.app.NotificationCompat;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.channels.DatagramChannel;

public class VPNService extends VpnService implements Runnable {
    private Thread mThread;
    private ParcelFileDescriptor mInterface;
    //a. Configure a builder for the interface.
    Builder builder = new Builder();
    private static final String CHANNEL_ID = "ShadowSocksConsoleNotificationChannel";

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel serviceChannel = new NotificationChannel(
                    CHANNEL_ID,
                    "ShadowSocksConsole Notification Channel",
                    NotificationManager.IMPORTANCE_DEFAULT
            );
            NotificationManager manager = getSystemService(NotificationManager.class);
            manager.createNotificationChannel(serviceChannel);
        }
    }

    private Notification CreateNotify(String message) {
        Intent notificationIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this,
                0, notificationIntent, 0);

        return new NotificationCompat.Builder(this, CHANNEL_ID)
                .setContentTitle("ShadowSocksConsole")
                .setContentText(message)
                .setSmallIcon(R.drawable.ic_launcher_background)
                .setContentIntent(pendingIntent)
                .build();
    }

    private void Notify(String message) {
        Notification not = CreateNotify(message);
        NotificationManager manager = getSystemService(NotificationManager.class);
        manager.notify(1, not);
    }

    private final String socketBaseName = "sock_path";
    private File LocalSocketPath() {
        return new File(ShadowSocksController.Get().CacheDir(), socketBaseName).getAbsoluteFile();
    }

    private boolean checkExistsSocket() {
        File directory = new File(ShadowSocksController.Get().CacheDir());
        File[] files = directory.listFiles();
        if (files != null) {
            for (int i = 0; i < files.length; i++) {
                if (files[i].getName().equals(socketBaseName))
                    return true;
            }
        }
        return false;
    }

    @Override
    public void run() {
        try {
            if (!ShadowSocksController.Get().Started())
                ShadowSocksController.Get().Start();
            {
                LocalSocketPath().delete();
            }
            ShadowSocksController.Get().appendLog("VPNService Started\n");
            boolean m_inited = false;
            boolean sock_connected = false;
            LocalSocket s = null;
            while (true) {
                //get packet with in
                //put packet to tunnel
                //get packet form tunnel
                //return packet with out
                //sleep is a must

                if (!sock_connected)
                    Thread.sleep(150);
                else
                    Thread.sleep(500);

                if (!ShadowSocksController.Get().Started())
                    break;
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


                    ShadowSocksController.Get().GetT2SParams().inited = true;
                    m_inited = true;
                    sock_connected = false;
                } else {
                    if (sock_connected && !checkExistsSocket()) {
                        m_inited = false;
                        try {
                            if (mInterface != null) {
                                mInterface.close();
                                mInterface = null;
                                ShadowSocksController.Get().appendLog("VPN Stopped\n");
                                this.Notify("VPN Stopped");
                            }
                        } catch (IOException e1) {

                        }
                        ShadowSocksController.Get().ClearT2SParams();
                        continue;
                    }
                    if (sock_connected)
                        continue;
                    try {
                        File path = LocalSocketPath();
                        s = new LocalSocket();
                        s.connect(new LocalSocketAddress(path.toString(), LocalSocketAddress.Namespace.FILESYSTEM));

                        FileDescriptor[] d = new FileDescriptor[1];
                        d[0] = mInterface.getFileDescriptor();
                        s.setFileDescriptorsForSend(d);
                        s.getOutputStream().write(42);
                        sock_connected = true;
                        ShadowSocksController.Get().appendLog("VPN Started\n");
                        this.Notify("VPN Started");
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
                getSystemService(NotificationManager.class).deleteNotificationChannel(CHANNEL_ID);
                ShadowSocksController.Get().appendLog("Notification Closed\n");
                stopForeground(true);
                stopSelf();
            } catch (Exception e) {

            }
        }
        ShadowSocksController.Get().appendLog("VPNService Finished\n");
    }


    // Services interface
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        createNotificationChannel();
        // Start a new session by creating a new thread.
        mThread = new Thread(this, "ShadowSocksConsole");

        if (ShadowSocksController.isNull()) {
            ShadowSocksController.Create(getApplicationInfo().nativeLibraryDir, getApplication().getFilesDir().getAbsolutePath(), getCacheDir().getAbsolutePath());
        }

        //start the service
        mThread.start();

        startForeground(1, CreateNotify("Service started"));

        ShadowSocksController.Get().appendLog("Notification created\n");

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
