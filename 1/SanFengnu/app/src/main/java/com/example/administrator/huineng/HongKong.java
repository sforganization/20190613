package com.example.administrator.huineng;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Color;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.Toast;

import com.example.administrator.huineng.R;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Timer;
import java.util.TimerTask;

public class HongKong extends AppCompatActivity {

    private  Button glassOpenLock[] = new Button[3];

    protected  int pressState[] = new int[3];   //
    protected  Button tryWear = null;

    protected void hideBottomUIMenu() {
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

    @Override
    protected void onDestroy() {
        super.onDestroy();
        int i;


        tick_task.cancel();
        serial_task.cancel();
        // 移除所有消息
        handler.removeCallbacksAndMessages(null);
        // 或者移除单条消息
        handler.removeMessages(0x1100);
    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){

        int msgNum = 0;
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            msgNum = msg.what;

            if(msgNum == 0x1100){
                //g_exit = true;   //只暂停， 停止线程会出现 问题

                stopTimer();
                handler.removeCallbacksAndMessages(null);
                Intent intent = new Intent();
                intent.setClass(HongKong.this, Advertisment.class);
                startActivityForResult(intent, 1);
            }else if(msgNum == 0x2200){
                //g_exit = true;   //只暂停， 停止线程会出现 问题
                stopTimer();
                handler.removeCallbacksAndMessages(null);
                Intent intent = new Intent();
                intent.setClass(HongKong.this, PasswdActivity.class);
                startActivityForResult(intent, 1);
            }
        }
    };

    volatile int movieTimes = 0;

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(requestCode == 1){
            movieTimes = 0;
            isReady = true;
            startTimer();
        }
    }

    Timer timer = null;
    TimerTask tick_task= null;
    TimerTask serial_task= null;
    volatile boolean isReady = true;

    private void startTimer(){
        if (timer == null) {
            timer = new Timer();
        }

        if (tick_task == null) {
            tick_task = new TimerTask() {
                @Override
                public void run() {

                            if(isReady == false)
                                return;

                            /**
                             *要执行的操作
                             */
                            movieTimes++;
                            if(movieTimes >= 10000)//屏幕没有操作跳到广告页面
                            {
                                movieTimes = 0;
                                isReady = false;
                                Message msg = Message.obtain();
                                handler.sendEmptyMessageDelayed((0x1100), 1);  //发送消息
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

        isReady = false;

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
        hideBottomUIMenu();


        Intent intent = getIntent();
        Bundle myBudle = this.getIntent().getExtras();

        setContentView(R.layout.activity_hong_kong);

        initViews();
        startTimer();
    }

    private void initViews() {
        //这是获得include中的控件
        tryWear = findViewById(R.id.hongkong_trywear);
        // 3.设置按钮点击事件
        tryWear .setOnClickListener(onClickListener);
    }


    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {

        boolean inputCheck2 = false;

        @Override
        public void onClick(View v) {

            movieTimes = 0;  //清空这个视频时间，有按下重新开始计时
            switch (v.getId()) {
                case R.id.hongkong_trywear:
                    movieTimes = 0;
                    isReady = false;
                    Message msg = Message.obtain();
                    handler.sendEmptyMessageDelayed((0x2200), 1);  //发送消息
                    break;

                default:
                    break;
            }
        }
    };
}
