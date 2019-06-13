package com.example.administrator.huineng;

import android.content.Intent;
import android.graphics.Color;
import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

import com.example.administrator.huineng.R;

public class Function_16_Activity extends AppCompatActivity {

    protected  int glass_num  = 0;
    protected  boolean inputcCheck  = true;
    private  View glass_01_include = null;
    private  View glass_02_include = null;
    private  View glass_03_include = null;
    private  View glass_04_include = null;
    private  View glass_05_include = null;
    private  View glass_06_include = null;
    private  View glass_07_include = null;
    private  View glass_08_include = null;
    private  View glass_09_include = null;
    private  View glass_10_include = null;
    private  View glass_11_include = null;
    private  View glass_12_include = null;
    private  View glass_13_include = null;
    private  View glass_14_include = null;
    private  View glass_15_include = null;
    private  Button glass_01_include_cicle = null;
    private  Button glass_02_include_cicle = null;
    private  Button glass_03_include_cicle = null;
    private  Button glass_04_include_cicle = null;
    private  Button glass_05_include_cicle = null;
    private  Button glass_06_include_cicle = null;
    private  Button glass_07_include_cicle = null;
    private  Button glass_08_include_cicle = null;
    private  Button glass_09_include_cicle = null;
    private  Button glass_10_include_cicle = null;
    private  Button glass_11_include_cicle = null;
    private  Button glass_12_include_cicle = null;
    private  Button glass_13_include_cicle = null;
    private  Button glass_14_include_cicle = null;
    private  Button glass_15_include_cicle = null;
    private  Button glass_return  = null;

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
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomUIMenu();
        setContentView(R.layout.activity_function_16_);
        initViews();
    }
    private void initViews() {
        //这是获得include中的控件
        glass_01_include = findViewById(R.id.function16_glass_01);
        glass_02_include = findViewById(R.id.function16_glass_02);
        glass_03_include = findViewById(R.id.function16_glass_03);
        glass_04_include = findViewById(R.id.function16_glass_04);
        glass_05_include = findViewById(R.id.function16_glass_05);
        glass_06_include = findViewById(R.id.function16_glass_06);
        glass_07_include = findViewById(R.id.function16_glass_07);
        glass_08_include = findViewById(R.id.function16_glass_08);
        glass_09_include = findViewById(R.id.function16_glass_09);
        glass_10_include = findViewById(R.id.function16_glass_10);
        glass_11_include = findViewById(R.id.function16_glass_11);
        glass_12_include = findViewById(R.id.function16_glass_12);
        glass_13_include = findViewById(R.id.function16_glass_13);
        glass_14_include = findViewById(R.id.function16_glass_14);
        glass_15_include = findViewById(R.id.function16_glass_15);
        //获取include中的子控件
        glass_01_include_cicle = glass_01_include.findViewById(R.id.include_cicle);
        glass_02_include_cicle = glass_02_include.findViewById(R.id.include_cicle);
        glass_03_include_cicle = glass_03_include.findViewById(R.id.include_cicle);
        glass_04_include_cicle = glass_04_include.findViewById(R.id.include_cicle);
        glass_05_include_cicle = glass_05_include.findViewById(R.id.include_cicle);
        glass_06_include_cicle = glass_06_include.findViewById(R.id.include_cicle);
        glass_07_include_cicle = glass_07_include.findViewById(R.id.include_cicle);
        glass_08_include_cicle = glass_08_include.findViewById(R.id.include_cicle);
        glass_09_include_cicle = glass_09_include.findViewById(R.id.include_cicle);
        glass_10_include_cicle = glass_10_include.findViewById(R.id.include_cicle);
        glass_11_include_cicle = glass_11_include.findViewById(R.id.include_cicle);
        glass_12_include_cicle = glass_12_include.findViewById(R.id.include_cicle);
        glass_13_include_cicle = glass_13_include.findViewById(R.id.include_cicle);
        glass_14_include_cicle = glass_14_include.findViewById(R.id.include_cicle);
        glass_15_include_cicle = glass_15_include.findViewById(R.id.include_cicle);
        glass_return = findViewById(R.id.function_return);

        glass_01_include_cicle.setOnClickListener(onClickListener);
        glass_02_include_cicle.setOnClickListener(onClickListener);
        glass_03_include_cicle.setOnClickListener(onClickListener);
        glass_04_include_cicle.setOnClickListener(onClickListener);
        glass_05_include_cicle.setOnClickListener(onClickListener);
        glass_06_include_cicle.setOnClickListener(onClickListener);
        glass_07_include_cicle.setOnClickListener(onClickListener);
        glass_08_include_cicle.setOnClickListener(onClickListener);
        glass_09_include_cicle.setOnClickListener(onClickListener);
        glass_10_include_cicle.setOnClickListener(onClickListener);
        glass_11_include_cicle.setOnClickListener(onClickListener);
        glass_12_include_cicle.setOnClickListener(onClickListener);
        glass_13_include_cicle.setOnClickListener(onClickListener);
        glass_14_include_cicle.setOnClickListener(onClickListener);
        glass_15_include_cicle.setOnClickListener(onClickListener);
        glass_return.setOnClickListener(onClickListener);
    }
    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (v.getId() == R.id.function_return) {
                //清理资源
                finish();
            }
            /*******************开锁按键处理**************************/
            //先取入include 布件
            //获得父控件的对象，然后获得父控件的id
            ViewGroup parent = (ViewGroup) v.getParent();
            inputcCheck = true;
            switch (parent.getId()) {
                case R.id.function16_glass_01:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 0;
                    }
                    break;

                case R.id.function16_glass_02:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 1;
                    }
                    break;

                case R.id.function16_glass_03:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 2;
                    }
                    break;
                case R.id.function16_glass_04:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 3;
                    }
                    break;
                case R.id.function16_glass_05:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 4;
                    }
                    break;
                case R.id.function16_glass_06:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 5;
                    }
                    break;
                case R.id.function16_glass_07:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 6;
                    }
                    break;
                case R.id.function16_glass_08:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 7;
                    }
                    break;
                case R.id.function16_glass_09:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 8;
                    }
                    break;
                case R.id.function16_glass_10:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 9;
                    }
                    break;
                case R.id.function16_glass_11:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 10;
                    }
                    break;
                case R.id.function16_glass_12:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 11;
                    }
                    break;
                case R.id.function16_glass_13:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 12;
                    }
                    break;
                case R.id.function16_glass_14:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 13;
                    }
                    break;
                case R.id.function16_glass_15:
                    if (v.getId() == R.id.include_cicle) {
                        glass_num = 14;
                    }
                    break;
                default:
                    inputcCheck = false;
                    break;
            }

            if(inputcCheck == true){

                Bundle bundle = new Bundle();
                Intent intent = new Intent(Function_16_Activity.this, ModeSetActivity.class);
                bundle.putInt("glass_num", glass_num);
                intent.putExtras(bundle);
                startActivity(intent);
            }
        }
    };
}
