package us.myprogram.shadowsocksconsole.ui.main;

import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.view.ViewPager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.TextView;
import android.support.annotation.Nullable;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.arch.lifecycle.Observer;
import android.arch.lifecycle.ViewModelProvider;

import us.myprogram.shadowsocksconsole.R;
import us.myprogram.shadowsocksconsole.ShadowSocksController;
import us.myprogram.shadowsocksconsole.databinding.FragmentMainBinding;

/**
 * A placeholder fragment containing a simple view.
 */
public class PlaceholderFragment extends Fragment {

    private static final String ARG_SECTION_NUMBER = "section_number";

    private FragmentMainBinding binding;

    public static PlaceholderFragment newInstance(int index) {
        PlaceholderFragment fragment = new PlaceholderFragment();
        Bundle bundle = new Bundle();
        bundle.putInt(ARG_SECTION_NUMBER, index);
        fragment.setArguments(bundle);
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

        binding = FragmentMainBinding.inflate(inflater, container, false);
        View root = binding.getRoot();

        Button startButton = (Button) binding.StartBatton;
        Button stopButton = (Button) binding.StopBatton;
        TextView statusInfo = (TextView) binding.controlStatus;
        if (startButton == null || stopButton == null || statusInfo == null)
            return root;

        if (!ShadowSocksController.isNull() && ShadowSocksController.Get().Started()) {
            if (ShadowSocksController.Get().isFullStarted()) {
                startButton.setClickable(false);
                startButton.setBackgroundColor(Color.GRAY);
                stopButton.setClickable(true);
                stopButton.setBackgroundColor(Color.RED);
                statusInfo.setText("Started");
                statusInfo.setTextColor(Color.GREEN);
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

        return root;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}