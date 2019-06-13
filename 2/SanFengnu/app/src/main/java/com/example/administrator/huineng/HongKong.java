package com.example.administrator.huineng;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
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
import android.widget.ImageView;
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
    protected  Button bu_return = null;

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

            if(msgNum == 0x2200){
                //g_exit = true;   //只暂停， 停止线程会出现 问题
                handler.removeCallbacksAndMessages(null);
                Intent intent = new Intent();

                Bundle bundle = new Bundle();
                bundle.putInt("glass_num", glass_num);
                intent.setClass(HongKong.this, PasswdActivity.class);

                intent.putExtras(bundle);
                startActivityForResult(intent, 1);
            }
        }
    };

    volatile  private int glass_num = 0;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomUIMenu();

        Intent intent = getIntent();
        Bundle bundle = this.getIntent().getExtras();
        glass_num = bundle.getInt("glass_num");

        setContentView(R.layout.activity_hong_kong);

        initViews();
        //startTimer();
    }

    private void initViews() {

        String myJpgPath =  "/mnt/sdcard/SanFeng/image/h1.png";
        ImageView jpgView= findViewById(R.id.hongkong_watchinfo);
        switch (glass_num)
        {
            case 0:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h1.png";
                break;
            case 1:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h2.png";
                break;
            default:
                break;
        }

        BitmapFactory.Options options = new BitmapFactory.Options();
        Bitmap bm = BitmapFactory.decodeFile(myJpgPath, null);
        jpgView.setImageBitmap(bm);

        //这是获得include中的控件
        tryWear = findViewById(R.id.hongkong_trywear);
        bu_return =findViewById(R.id.hongkong_ruturn);
        // 3.设置按钮点击事件
        tryWear .setOnClickListener(onClickListener);
        bu_return .setOnClickListener(onClickListener);
    }


    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {

        boolean inputCheck2 = false;

        @Override
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.hongkong_trywear:
                    Message msg = Message.obtain();
                    handler.sendEmptyMessageDelayed((0x2200), 1);  //发送消息
                    break;
                case R.id.hongkong_ruturn:
                    finish();
                    break;

                default:
                    break;
            }
        }
    };
}
