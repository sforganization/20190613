package com.example.administrator.huineng;

import android.annotation.SuppressLint;
import android.content.Intent;
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

public class SelectActivity extends AppCompatActivity {

    protected  int glass_num = 0;


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

    volatile int movieTimes = 0;

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(requestCode == 1){
            movieTimes = 0;
            isReady = true;
            startTimer();
        }
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


            stopTimer();
            handler.removeCallbacksAndMessages(null);
            Intent intent = new Intent();

            Bundle bundle = new Bundle();
            bundle.putInt("glass_num", glass_num);

//            Bundle bundle = new Bundle();
//            Intent intent = new Intent(Function_16_Activity.this, ModeSetActivity.class);
//            bundle.putInt("glass_num", glass_num);
//            intent.putExtras(bundle);
//            startActivity(intent);

            if(msgNum == 0x1100){ //广告
                //g_exit = true;   //只暂停， 停止线程会出现 问题
                intent.setClass(SelectActivity.this, Advertisment.class);
            }else if(msgNum == 0xaa00){
                intent.setClass(SelectActivity.this, HongKong.class);
            }else if(msgNum == 0xbb00){
                intent.setClass(SelectActivity.this, HongKong.class);
            }else if(msgNum == 0xcc00){  //设置界面
                intent.setClass(SelectActivity.this, PasswdActivity.class);
            }

            intent.putExtras(bundle);
            startActivityForResult(intent, 1);
        }
    };

    volatile private int watchNum = 0;
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

    private Button optionSet = null;
    private Button watch_a = null;
    private Button watch_b = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomKey();

        Bundle bundle = this.getIntent().getExtras();
        glass_num = bundle.getInt("glass_num");
        setContentView(R.layout.activity_select);

        initViews();
        startTimer();
    }

    private void initViews() {
        //这是获得include中的控件
        optionSet = findViewById(R.id.select_set);
        watch_a  = findViewById(R.id.select_a);
        watch_b  = findViewById(R.id.select_b);
        // 3.设置按钮点击事件
        optionSet .setOnClickListener(onClickListener);
        watch_a .setOnClickListener(onClickListener);
        watch_b .setOnClickListener(onClickListener);
    }

    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {

        boolean inputCheck2 = false;

        @Override
        public void onClick(View v) {

            movieTimes = 0;  //清空这个视频时间，有按下重新开始计时
            isReady = false;
            Message msg = Message.obtain();

            switch (v.getId()) {
                case R.id.select_a:
                    glass_num = 0;
                    handler.sendEmptyMessageDelayed((0xaa00), 1);  //发送消息
                    break;

                case R.id.select_b:
                    glass_num = 1;
                    handler.sendEmptyMessageDelayed((0xbb00), 1);  //发送消息
                    break;

                case R.id.select_set:
                    glass_num = 0xff;  //模式设置
                    handler.sendEmptyMessageDelayed((0xcc00), 1);  //发送消息
                    break;
                default:
                    break;
            }
        }
    };

}
