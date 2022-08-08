package us.myprogram.shadowsocksconsole.ui.main;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.webkit.WebViewClient;

import us.myprogram.shadowsocksconsole.R;
import us.myprogram.shadowsocksconsole.databinding.FragmentMainPageBinding;

/**
 * A placeholder fragment containing a simple view.
 */
public class PageMain extends Fragment {

    private FragmentMainPageBinding binding;
    private WebView viewer = null;

    public static PageMain newInstance() {
        PageMain fragment = new PageMain();
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {

        binding = FragmentMainPageBinding.inflate(inflater, container, false);
        View root = binding.getRoot();

        WebView v = binding.webview;
        if (v != null) {
            v.setWebViewClient(new WebViewClient() {
                @SuppressWarnings("deprecation")
                @Override
                public boolean shouldOverrideUrlLoading(WebView webView, String url) {
                    webView.loadUrl(url);
                    return true;
                }
            });
            v.getSettings().setJavaScriptEnabled(true);
            v.getSettings().setJavaScriptCanOpenWindowsAutomatically(true);
            v.loadUrl("http://localhost:35080/");
        }

        return root;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}