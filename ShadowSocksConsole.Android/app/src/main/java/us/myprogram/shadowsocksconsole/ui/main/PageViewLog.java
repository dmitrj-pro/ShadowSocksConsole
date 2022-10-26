package us.myprogram.shadowsocksconsole.ui.main;

import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import java.util.Timer;
import java.util.TimerTask;

import us.myprogram.shadowsocksconsole.R;
import us.myprogram.shadowsocksconsole.ShadowSocksConsoleProcess;
import us.myprogram.shadowsocksconsole.ShadowSocksController;
import us.myprogram.shadowsocksconsole.databinding.ShowLogBinding;

/**
 * A placeholder fragment containing a simple view.
 */
public class PageViewLog extends Fragment {

    private static final String ARG_SECTION_NUMBER = "section_number";

    private ShowLogBinding binding;
    private TextView textView = null;
    private TimerTask timerFunc = null;
    private Timer timer = null;
    private boolean textViewInited = false;

    public static PageViewLog newInstance() {
        PageViewLog fragment = new PageViewLog();
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

        binding = ShowLogBinding.inflate(inflater, container, false);
        View root = binding.getRoot();

        this.textView = binding.loggingLabel;
        timerFunc = new TimerTask() {
            @Override
            public void run() {
                updateLogging();
            }
        };
        timer = new Timer();
        timer.schedule(timerFunc, 0, 1000);
        return root;
    }

    public void updateLogging() {
        if (getActivity() == null)
            return;
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                TextView t = (TextView) getActivity().findViewById(R.id.logging_label);
                if (t == null)
                    return;
                if (!textViewInited) {
                    t.setMovementMethod(new ScrollingMovementMethod());
                    //textViewInited = true;
                }
                if (!ShadowSocksController.isNull())
                    t.setText(ShadowSocksController.Get().getLogging());
            }
        });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}