package us.myprogram.shadowsocksconsole.ui.main;

import android.content.Context;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.view.MotionEvent;

public class CustomPager extends ViewPager {
    private Boolean disable = false;
    public CustomPager(Context context) {
        super(context);
    }
    public CustomPager(Context context, AttributeSet attrs){
        super(context,attrs);
    }
    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        if (this.getCurrentItem() == 0)
            return false;
        return !disable && super.onInterceptTouchEvent(event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (this.getCurrentItem() == 0)
            return false;
        return !disable && super.onTouchEvent(event);
    }

    public void disableScroll(Boolean disable){
        //When disable = true not work the scroll and when disble = false work the scroll
        this.disable = disable;
    }
}
