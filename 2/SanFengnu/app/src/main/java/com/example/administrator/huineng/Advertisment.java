package com.example.administrator.huineng;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Color;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.MediaController;
import android.widget.Toast;
import android.widget.VideoView;

import com.example.administrator.huineng.R;

public class Advertisment extends AppCompatActivity {

    String filePath = "/mnt/sdcard/SanFeng/movies/";


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

//        Window _window;
//        _window = getWindow();
//
//        WindowManager.LayoutParams params = _window.getAttributes();
//        params.systemUiVisibility = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
//        _window.setAttributes(params);
    }


    //本地的视频  需要添加一个 ,mp4 视频
    String videoUrl1 = filePath + "lanbo.mp4";
    Uri uri;
    private VideoView videoView;

    @Override
    protected void onStop() {

        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if(videoView!=null){
            videoView.pause();
            videoView.stopPlayback();
        }
        Intent intentTemp = new Intent();
        intentTemp.putExtra("timerState", 1);
        setResult(1, intentTemp);
        finish();
    }

    public void onClick(View v) {
        if(videoView!=null){
            videoView.pause();
            videoView.stopPlayback();
        }
        onStop();
    }

    MediaController mc = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        hideBottomUIMenu();
        setContentView(R.layout.activity_advertisment);


        uri = Uri.parse( videoUrl1 );

        videoView = this.findViewById(R.id.videoView );

        //设置视频控制器
        mc = new MediaController(this);
        mc.setVisibility(View.INVISIBLE); //隐藏进度条
        videoView.setMediaController(mc);

        //播放完成回调
        videoView.setOnCompletionListener( new MyPlayerOnCompletionListener());

        //设置视频路径
        videoView.setVideoURI(uri);

        //开始播放视频
        videoView.start();

        videoView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                videoView.pause();
                videoView.stopPlayback();
                finish();
                return true;
            }
        });

    }


///*    @SuppressLint("HandlerLeak")
//    private Handler handler = new Handler(){
//
//        int msgNum = 0;
//        public void handleMessage(Message msg) {
//            super.handleMessage(msg);
//            msgNum = (int)msg.what;
//
//            if(msgNum == 0x2200){
//                hideBottomUIMenu();
//            }
//
//            if(msgNum == 0x3300){
//                //设置视频控制器
//*//*                mc.setVisibility(View.INVISIBLE); //隐藏进度条
//                videoView.setMediaController(mc);
//
//                videoView.pause();*//*
//                //videoView.stopPlayback();
//                //设置视频路径
//                videoView.setVideoURI(uri);
//                //或 //mVideoView.setVideoPath(Uri.parse(_filePath));
//                videoView.start();
//            }
//        }
//    };*/

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {  //重写android虚拟按键
        videoView.pause();
        videoView.stopPlayback();
        finish();
        return true;
    }

    class MyPlayerOnCompletionListener implements MediaPlayer.OnCompletionListener {
        @Override
        public void onCompletion(MediaPlayer mp) {
            Toast.makeText( Advertisment.this, "播放完成了", Toast.LENGTH_SHORT).show();
            Message msg = Message.obtain();
            //设置视频路径
            videoView.setVideoURI(uri);
            //或 //mVideoView.setVideoPath(Uri.parse(_filePath));
            videoView.start();
        }
    }
}


