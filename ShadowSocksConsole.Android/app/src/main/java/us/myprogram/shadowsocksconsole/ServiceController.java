package us.myprogram.shadowsocksconsole;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class ServiceController  extends Service {
    public void onCreate() {
        super.onCreate();
    }
    public IBinder onBind(Intent intent) {
        return null;
    }
}
