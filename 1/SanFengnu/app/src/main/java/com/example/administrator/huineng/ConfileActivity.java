package com.example.administrator.huineng;

import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;

import com.example.administrator.huineng.R;

public class ConfileActivity extends AppCompatActivity {


    protected  int glass_num = 0;

    protected void hideBottomKey(){
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
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomKey();

        Bundle bundle = this.getIntent().getExtras();
        glass_num = bundle.getInt("glass_num");

        setContentView(R.layout.activity_confile);

        Button confile_tryWearButton = findViewById(R.id.confile_tryWearButton2);
        confile_tryWearButton.setOnClickListener(new View.OnClickListener(){
            public void onClick(View v){

                Intent intent = new Intent();
                Bundle myBudle=new Bundle();
                myBudle.putInt("glass_num", glass_num);
                intent.putExtras(myBudle);

                intent.setClass(ConfileActivity.this, PasswdActivity.class);
                ConfileActivity.this.startActivity(intent);
                //finish();
            }
        });

        Button confile_return = findViewById(R.id.confile_return_button);
        confile_return.setOnClickListener(new View.OnClickListener(){
            public void onClick(View v){
                //清理返回
                finish();
            }
        });

        String myJpgPath =  "/mnt/sdcard/SanFeng/image/h1.jpg";
        ImageView jpgView= findViewById(R.id.glass_info);
        switch (glass_num)
        {
            case 0:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h1.png";
                break;
            case 1:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h2.png";
                break;
            case 2:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h3.png";
                break;
            case 3:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h4.png";
                break;
            case 4:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h5.png";
                break;
            case 5:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h6.png";
                break;
            case 6:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h7.png";
                break;
            case 7:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h8.png";
                break;
            case 8:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h9.png";
                break;
            case 9:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h10.png";
                break;
            case 10:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h11.png";
                break;
            case 11:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h12.png";
                break;
            case 12:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h13.png";
                break;
            case 13:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h14.png";
                break;
            case 14:
                myJpgPath = "/mnt/sdcard/SanFeng/image/h15.png";
                break;
            default:
                break;
        }

        BitmapFactory.Options options = new BitmapFactory.Options();
        Bitmap bm = BitmapFactory.decodeFile(myJpgPath, null);
        jpgView.setImageBitmap(bm);
    }
}
