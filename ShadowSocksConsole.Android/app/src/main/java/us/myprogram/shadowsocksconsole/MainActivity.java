package us.myprogram.shadowsocksconsole;

import android.content.Intent;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.net.VpnService;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.design.widget.TabLayout;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.TextView;

import java.util.Timer;
import java.util.TimerTask;

import us.myprogram.shadowsocksconsole.ui.main.SectionsPagerAdapter;
import us.myprogram.shadowsocksconsole.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding binding;

    private TimerTask timerFunc = null;
    private Timer timer = null;
    private boolean webviewerChanged = false;
    private boolean startedView = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        SectionsPagerAdapter sectionsPagerAdapter = new SectionsPagerAdapter(this, getSupportFragmentManager());
        ViewPager viewPager = binding.viewPager;
        viewPager.setAdapter(sectionsPagerAdapter);
        TabLayout tabs = binding.tabs;
        tabs.setupWithViewPager(viewPager);

        //ShadowSocksController.Create(getApplicationInfo().nativeLibraryDir, this.getFilesDir().getAbsolutePath());

        timerFunc = new TimerTask() {
            @Override
            public void run() {
                updateButton();
            }
        };
        timer = new Timer();
        timer.schedule(timerFunc, 0, 1000);

    }

    private void installWebView(){
        if (webviewerChanged)
            return;
        WebView v = (WebView) findViewById(R.id.webview);
        v.setWebViewClient(new WebViewClient()
        {
            @SuppressWarnings("deprecation")
            @Override
            public boolean shouldOverrideUrlLoading(WebView webView, String url)
            {
                webView.loadUrl(url);
                installWebView();
                return true;
            }
        });
        v.getSettings().setJavaScriptEnabled(true);
        v.getSettings().setJavaScriptCanOpenWindowsAutomatically(true);
        webviewerChanged = true;
    }

    public void updateButton() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Button startButton = (Button) findViewById(R.id.StartBatton);
                Button stopButton = (Button) findViewById(R.id.StopBatton);
                TextView statusInfo = (TextView) findViewById(R.id.control_status);
                if (startButton == null || stopButton == null || statusInfo == null)
                    return;
                if (!startedView) {
                    ViewPager pager = (ViewPager) findViewById(R.id.view_pager);
                    if (pager == null)
                        return;
                    int page = 1;
                    if (!ShadowSocksController.isNull() && ShadowSocksController.Get().isFullStarted())
                        page = 0;
                    pager.setCurrentItem(page);
                    startedView = true;
                }

                if (!ShadowSocksController.isNull() && ShadowSocksController.Get().Started()) {
                    if (ShadowSocksController.Get().isFullStarted() && !stopButton.isClickable()) {
                        startButton.setClickable(false);
                        startButton.setBackgroundColor(Color.GRAY);
                        stopButton.setClickable(true);
                        stopButton.setBackgroundColor(Color.RED);
                        statusInfo.setText("Started");
                        statusInfo.setTextColor(Color.GREEN);
                        WebView v = (WebView) findViewById(R.id.webview);
                        installWebView();
                        if (v != null)
                            v.loadUrl("http://localhost:35080/");
                    } else {
                        if (!ShadowSocksController.Get().isFullStarted()) {
                            statusInfo.setText("Starting");
                            statusInfo.setTextColor(Color.MAGENTA);
                            startButton.setClickable(false);
                            startButton.setBackgroundColor(Color.GRAY);
                        }
                    }
                } else {
                    if (stopButton.isClickable()) {
                        statusInfo.setText("Stoped");
                        statusInfo.setTextColor(Color.RED);
                        startButton.setClickable(true);
                        startButton.setBackgroundColor(Color.GREEN);
                        stopButton.setClickable(false);
                        stopButton.setBackgroundColor(Color.GRAY);
                    }
                }
            }
        });


    }
    public void onButtonStart(View view) {
        if (!ShadowSocksController.isNull() && ShadowSocksController.Get().Started())
            return;
        //ShadowSocksController.Get().Start();
        startService(new Intent(this.getApplicationContext(), VPNService.class));
        updateButton();
    }
    public void onButtonReload(View view) {
        WebView v = (WebView) findViewById(R.id.webview);
        if (v != null) {
            v.loadUrl("http://localhost:35080/");
            installWebView();
        }
    }
    public void onButtonStop(View view) {
        if (ShadowSocksController.isNull() || !ShadowSocksController.Get().Started())
            return;
        ShadowSocksController.Get().Stop();
        updateButton();
    }
    public void onButtonKill(View view) {
        if (ShadowSocksController.isNull() || !ShadowSocksController.Get().Started())
            return;
        ShadowSocksController.Get().KILL();
        updateButton();
    }
    public void onButtonGrant(View view) {
        Intent intent = VpnService.prepare(getApplicationContext());
        if (intent != null) {
            startActivityForResult(intent, 0);
        } else {
            onActivityResult(0, RESULT_OK, null);
        }
    }
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
        }
    }
}