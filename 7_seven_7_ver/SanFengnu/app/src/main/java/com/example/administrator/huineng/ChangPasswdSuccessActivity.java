package com.example.administrator.huineng;

import android.annotation.SuppressLint;
import android.graphics.Color;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

import java.util.Timer;
import java.util.TimerTask;

public class ChangPasswdSuccessActivity extends AppCompatActivity {
    protected void hideBottomKey() {
        if (Build.VERSION.SDK_INT > 11 && Build.VERSION.SDK_INT < 19) {
            View v = this.getWindow().getDecorView();
            v.setSystemUiVisibility(View.GONE);
        } else if (Build.VERSION.SDK_INT >= 19) {
            Window window = getWindow();
            int uiOptions =View.SYSTEM_UI_FLAG_LOW_PROFILE
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
                    | View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
                    | View.SYSTEM_UI_FLAG_IMMERSIVE;
            window.getDecorView().setSystemUiVisibility(uiOptions);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                window.setStatusBarColor(Color.TRANSPARENT);
            }
            window.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
        }

        Window _window;
        _window = getWindow();

        WindowManager.LayoutParams params = _window.getAttributes();
        params.systemUiVisibility = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
        _window.setAttributes(params);
    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {      //判断标志位
                case 0x5500:
                    /**
                     获取数据，更新UI
                     */

                    handler.removeCallbacksAndMessages(null);
                    stopTimer();
                    finish();
                    break;

                default:
                    break;
            }
        }
    };

    volatile int waitTime = 0;
    Timer timer = null;
    TimerTask tick_task= null;

    private void startTimer(){
        if (timer == null) {
            timer = new Timer();
        }

        if (tick_task == null) {
            tick_task = new TimerTask() {
                @Override
                public void run() {
                    /**
                     *要执行的操作
                     */
                    waitTime++;
                    if(waitTime >= 3000)//屏幕没有操作跳到广告页面
                    {
                        waitTime = 0;
                        Message msg = Message.obtain();
                        handler.sendEmptyMessageDelayed((0x5500), 1);  //发送消息
                    }
                }
            };
        }

        if(timer != null)
        {
            if(tick_task != null ) {
                timer.schedule(tick_task, 10, 1);//1ms后执行Tick   1ms 的tick
            }

        }
    }

    private void stopTimer(){
        if (tick_task != null) {
            tick_task.cancel();
            tick_task = null;
        }

        if (timer != null) {
            timer.cancel();
            timer = null;
        }
        System.gc();
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomKey();
        setContentView(R.layout.activity_chang_passwd_success);

        Button passwd_return_button = findViewById(R.id.chsuccess_ruturn);
        passwd_return_button.setOnClickListener(onClickListener);

        startTimer();
    }

    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.chsuccess_ruturn:
                    waitTime = 0;
                    Message msg = Message.obtain();
                    handler.sendEmptyMessageDelayed((0x5500), 1);  //发送消息
                    break;
                default:
                    break;
            }
        }
    };
}
