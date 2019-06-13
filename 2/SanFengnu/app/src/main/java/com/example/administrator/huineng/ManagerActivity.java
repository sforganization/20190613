package com.example.administrator.huineng;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Color;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.example.administrator.huineng.R;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.util.Timer;
import java.util.TimerTask;

public class ManagerActivity extends AppCompatActivity {

    protected  int glass_num = 0;
    protected  int inputCnt = 0;   //输入记数
    protected  String inputNum = new String();   //输入值
    protected  boolean inputCheck = true;   //输入是否有效值
    protected  boolean passwdState = false;   //密码状态

    public static   String g_passwd = new String("");

    TextView input_1 ;
    TextView input_2 ;
    TextView input_3 ;
    TextView input_4 ;
    TextView input_5 ;
    TextView input_6 ;
    TextView error_info;

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

    private com.example.x6.serial.SerialPort serialttyS0;
    private InputStream ttyS0InputStream;
    private OutputStream ttyS0OutputStream;
    /* 打开串口 */
    private void init_serial() {
        try {
            Log.d("fff", "x 初始化串口。。。。。。");
            serialttyS0 = new com.example.x6.serial.SerialPort(new File("/dev/ttyS2"),115200,0, 5);
//            serialttyS0 = new com.example.x6.serial.SerialPort(new File("/dev/ttyS0"),115200,0, 5);

            Log.d("fff", "x 初始化串口。。。。。。赋值输入流");
            ttyS0InputStream = serialttyS0.getInputStream();
            Log.d("fff", "x 初始化串口。。。。。。赋值输出流");
            ttyS0OutputStream = serialttyS0.getOutputStream();
            Log.d("fff", "x 初始化串口。。。。。。退出");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /*
    //    包头 + 命令 +参数[3] +数据[3]    +  校验和 + 包尾
    //    包头：0XAA
    //    命令： CMD_UPDATE   、 CMD_READY
    //    参数： 地址，LED模式，增加删除指纹ID,
    //    数据：（锁开关 + 方向 + 时间）
    //    校验和：
    //    包尾： 0X55
    //    参数传递过来是命令+参数+数据  data[7]
     */
    public byte[] makeStringtoFramePackage(byte[] data)
    {
        //在时间byte[]前后添加一些package校验信息
        int dataLength   = 8;
        byte CheckSum     = 0;
        byte[] terimalPackage=new byte[10];

        //装填信息
        //时间数据包之前的信息
        terimalPackage[0] = (byte)0xAA;			   //包头
        terimalPackage[1] = data[0];         //cmd 命令
        terimalPackage[2] = data[1];         //参数0  地址，LED模式，增加删除指纹ID,
        terimalPackage[3] = data[2];         //
        terimalPackage[4] = data[3];         //
        terimalPackage[5] = data[4];         //数据  （锁开关 + 方向 + 时间）
        terimalPackage[6] = data[5];         //
        terimalPackage[7] = data[6];         //

        //计算校验和
        //转化为无符号进行校验
        for(int dataIndex = 0; dataIndex < 8; dataIndex++)
        {
            CheckSum += terimalPackage[dataIndex];
        }
        terimalPackage[8] = (byte)(~CheckSum);          //检验和
        terimalPackage[9] = (byte)0x55;            //包尾
        return terimalPackage;
    }

    private void sendFingerENPack() {  //发送包
        byte[] temp_bytes = new byte[]{0x09, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00};  // 09 使能指纹接收命令，0x11保留参数
        byte[] send = makeStringtoFramePackage(temp_bytes);
        /*串口发送字节*/
        try {
            ttyS0OutputStream.write(send);
            //ttyS1InputStream.read(send);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void sendFingerDisablePack() {  //发送包
        byte[] temp_bytes = new byte[]{0x0A, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00};  // 0A 不使能指纹接收命令，0x11保留参数
        byte[] send = makeStringtoFramePackage(temp_bytes);
        /*串口发送字节*/
        try {
            ttyS0OutputStream.write(send);
            //ttyS1InputStream.read(send);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    //    包头 + 地址 + 命令 + 数据包[48] + 校验和 + 包尾
    //    包头：0XAA
    //    手表地址：0X8~：最高位为1表示全部                       ~:表示有几个手表
    //              0x01：第一个手表
    //    命令：
    //    数据：（锁开关 + 方向 + 时间）* 16 = 48
    //    校验和：
    //    包尾： 0X55

    private byte[] recvData = new byte[128];    //每次最大是32位
    private byte[] recvArry = new byte[256];    //设置缓存大概4级    256/ 53 ~ 4
    private byte ackCheck= (byte)0xFF;  //一个字节的应答判断
    private int i= 0;
    private int index= 0;
    private int cnt= 0;
    private int sizeRec= 0;
    private byte[] glassData = new byte[50];    //一个完整数据包
    private byte checkSum = 0;
    volatile  private boolean exit = false;

    ManagerActivity.WorkThread myThread = new ManagerActivity.WorkThread();  //串口接收指纹数据

    protected int jungleRecPack() {   //

        for(index = 0; index < 255; index++)
        {
            if(recvArry[index] == (byte)0xAA)  //如果是包头
            {
                break;
            }
        }

        if(index == 255) //出错
            return -1;
        checkSum = 0;
        for(i = 0 ; i < 52; i++) //和校验
        {
            checkSum += recvArry[index + i];
        }
        checkSum += recvArry[index + i];
        if(checkSum != (byte)0x55 - 1)
            return -1; //


        //应答地址 0xFF
        if(recvArry[index + 1] == (byte)0xff)        {
            if(recvArry[index + 2]  == (byte)0x01)   //0x01 指纹应答类型
            {
                ackCheck = recvArry[index + 3];
                return 0;
            }
        }

        ackCheck = (byte) 0xFF;
        return 0;
    }

    protected int checkRecPack() {   //串口接收数据

        int tmp_cnt = 0;
        int index = 0;

        if( exit == true)
            return -1;

        sendFingerENPack();        /*串口发送使能指纹输入包*/
        try {
            while(exit != true)
            {
                sizeRec = 0;
                for(i = 0; i < 128; i++)   //清空数组
                    recvData[i] = 0;

                do {
                    cnt = -1;
                    if(ttyS0InputStream != null)
                        cnt = ttyS0InputStream.read(recvData);

                    Log.d("fff", " 2 接收到数据。。。。。。");
                    if(cnt != -1) {
                        System.arraycopy(recvData, 0, recvArry, sizeRec, cnt);
                        sizeRec += cnt;
                    }

//                    Log.e("fff", "接收到数据。。。。。。");
//                    if(sizeRec == 0) {  //前面还没有收到包头
//                        for (index = 0; index < cnt; index++) {
//                            if (recvData[index] == (byte) 0xAA)  //如果是包头
//                            {
//                                break;
//                            }
//                        }
//                    }
//
//                    if(index == cnt) {//没有找到包头
//                        sizeRec = 0;
//                    }else {
//                        System.arraycopy(recvData, index, recvArry, sizeRec, cnt - index);
//                        sizeRec += cnt;
//                    }

                } while ((sizeRec < 53) && (exit == false));  //少于一个包数据  //或者已经退出

                if( exit == true)
                    return -1;

                if(jungleRecPack() == 0) //检查参数合法性,数据合法，但是不一定是正确应答
                {
                    break;
                }
                else  //出错重新发
                {
                    TimerTask task = new TimerTask() {
                        @Override
                        public void run() {
                            /**
                             *要执行的操作
                             */
                            sendFingerENPack();        /*串口重新发送包*/
                            this.cancel();
                        }
                    };

                    Timer timer = new Timer();
                    timer.schedule(task, 200);//300ms后执行TimeTask的run方法
                }
                Log.d("fff", "onCreate: dd000");
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return 0;
    }

    public class WorkThread extends Thread {

        @Override
        public void run() {
            super.run();
            /**
             耗时操作
             */

            init_serial();          //初始化串口
            Log.d("fff", "x 线程开启。。。。。。");
            while(!isInterrupted()) {
                Log.d("fff", "x 线程没有中断。。。。。。");
                    checkRecPack();

                    if (exit == true)   //密码登录成功，退出
                        return;

                    Log.d("fff", "线程还在继续。。。。。。");

                    //从全局池中返回一个message实例，避免多次创建message（如new Message）
                    Message msg = Message.obtain();
                    //            msg.obj = glassData;
                    //            msg.what=1;   //标志消息的标志

                    if (ackCheck == 0) //判断应答是否正确
                    {
                        handler.sendEmptyMessageDelayed(3, 1);  //发送消息
                    } else if (ackCheck == 1) //判断应答错误
                    {
                        handler.sendEmptyMessageDelayed(4, 1);  //发送指纹识别错误消息
                    } else {  //异常

                    }
            }

        }
    }

    //    @SuppressLint("HandlerLeak")
    @Override
    protected void onDestroy() { //Activity退出时会调用
        super.onDestroy();
        exit = true;

        sendFingerDisablePack();

        if (myThread != null) {
            Log.d("aaa", "onDestroy 发送线程停止interrupt。。。。。。");
            myThread.interrupt();
        }

        //关闭串口
        try {
            ttyS0InputStream.close();
            Log.d("aaa", "onDestroy 关out。。。。。。");
        } catch (IOException e) {
            e.printStackTrace();
        }

        try {
            ttyS0OutputStream.close();
            Log.d("aaa", "onDestroy 关in。。。。。。");
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (serialttyS0 != null) {
            serialttyS0.close();
            Log.d("fff", "串口不为空 关闭串口。。。。。。");
            serialttyS0 = null;
        }

        // 移除所有消息
        handler.removeCallbacksAndMessages(null);
        // 或者移除单条消息
        handler.removeMessages(1);
    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {      //判断标志位
                case 3:
                    /**
                     获取数据，更新UI
                     */
                    //密码正确，写入，且更新到输入框

                    input_6.setVisibility(View.VISIBLE);
                    input_5.setVisibility(View.VISIBLE);
                    input_4.setVisibility(View.VISIBLE);
                    input_3.setVisibility(View.VISIBLE);
                    input_2.setVisibility(View.VISIBLE);
                    input_1.setVisibility(View.VISIBLE);

                    manage_finger.setClickable(true);
                    manage_open_lock.setClickable(true);
                    manage_function.setClickable(true);

                    manage_finger.setVisibility(View.VISIBLE);
                    manage_open_lock.setVisibility(View.VISIBLE);
                    manage_function.setVisibility(View.VISIBLE);

                    manage_finger_txt.setVisibility(View.VISIBLE);
                    manage_open_lock_txt.setVisibility(View.VISIBLE);
                    manage_function_txt.setVisibility(View.VISIBLE);

                    error_info.setText("密码正确!");
                    error_info.setVisibility(View.VISIBLE);
                    passwdState = true;
                    break;
                case 4:
                    /**
                     获取数据，更新UI
                     */
                    input_6.setVisibility(View.VISIBLE);
                    input_5.setVisibility(View.VISIBLE);
                    input_4.setVisibility(View.VISIBLE);
                    input_3.setVisibility(View.VISIBLE);
                    input_2.setVisibility(View.VISIBLE);
                    input_1.setVisibility(View.VISIBLE);

                    manage_finger.setClickable(false);
                    manage_open_lock.setClickable(false);
                    manage_function.setClickable(false);

                    manage_finger.setVisibility(View.INVISIBLE);
                    manage_open_lock.setVisibility(View.INVISIBLE);
                    manage_function.setVisibility(View.INVISIBLE);

                    manage_finger_txt.setVisibility(View.INVISIBLE);
                    manage_open_lock_txt.setVisibility(View.INVISIBLE);
                    manage_function_txt.setVisibility(View.INVISIBLE);

                    error_info.setText("未识别的指纹!");
                    error_info.setVisibility(View.VISIBLE);
                    break;
            }

            if(inputNum.length() > 0) {
                inputNum = "";
                inputCnt = 0;
            }
        }
    };

    Button manage_finger = null;
    Button manage_open_lock =null;
    Button manage_function =null;
    Button manage_finger_txt = null;
    Button manage_open_lock_txt =null;
    Button manage_function_txt =null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomKey();

        Log.d("fff", "x 进入oncreate。。。。。。");
        exit = false;
        setContentView(R.layout.activity_manager);

        String filePath = "/mnt/sdcard/SanFeng/data/";
        String fileName = "loginfo";
        makeFilePath(filePath, fileName);

        //writeTxtToFile("666666", filePath, fileName);
        readFile(filePath, fileName);

         input_1 = findViewById(R.id.manage_input_1);
         input_2 = findViewById(R.id.manage_input_2);
         input_3 = findViewById(R.id.manage_input_3);
         input_4 = findViewById(R.id.manage_input_4);
         input_5 = findViewById(R.id.manage_input_5);
         input_6 = findViewById(R.id.manage_input_6);
         error_info = findViewById(R.id.manage_error);

        Button input_00_Button = findViewById(R.id.manage_digit_00);
        Button input_01_Button = findViewById(R.id.manage_digit_01);
        Button input_02_Button = findViewById(R.id.manage_digit_02);
        Button input_03_Button = findViewById(R.id.manage_digit_03);
        Button input_04_Button = findViewById(R.id.manage_digit_04);
        Button input_05_Button = findViewById(R.id.manage_digit_05);
        Button input_06_Button = findViewById(R.id.manage_digit_06);
        Button input_07_Button = findViewById(R.id.manage_digit_07);
        Button input_08_Button = findViewById(R.id.manage_digit_08);
        Button input_09_Button = findViewById(R.id.manage_digit_09);
        Button manage_del_button = findViewById(R.id.manage_del_button);
        Button manage_return_button = findViewById(R.id.manage_ruturn);
        manage_finger = findViewById(R.id.manager_finger_button);
        manage_open_lock = findViewById(R.id.manager_open_lock_button);
        manage_function = findViewById(R.id.manager_function_button);
        manage_finger_txt = findViewById(R.id.manager_finger_txt);
        manage_open_lock_txt = findViewById(R.id.manager_lock_txt);
        manage_function_txt = findViewById(R.id.manage_function_txt);
        // 3.设置按钮点击事件
        input_00_Button.setOnClickListener(onClickListener);
        input_01_Button.setOnClickListener(onClickListener);
        input_02_Button.setOnClickListener(onClickListener);
        input_03_Button.setOnClickListener(onClickListener);
        input_04_Button.setOnClickListener(onClickListener);
        input_05_Button.setOnClickListener(onClickListener);
        input_06_Button.setOnClickListener(onClickListener);
        input_07_Button.setOnClickListener(onClickListener);
        input_08_Button.setOnClickListener(onClickListener);
        input_09_Button.setOnClickListener(onClickListener);
        manage_del_button.setOnClickListener(onClickListener);
        manage_return_button.setOnClickListener(onClickListener);
        manage_finger.setOnClickListener(onClickListener);
        manage_open_lock.setOnClickListener(onClickListener);
        manage_function.setOnClickListener(onClickListener);

        TimerTask task = new TimerTask() {
            @Override
            public void run() {
                /**
                 *要执行的操作
                 */
                myThread.start();        /*延时一段时间再开启线程*/
                this.cancel();
            }
        };
        Timer timer = new Timer();
        timer.schedule(task, 200);//300ms后执行TimeTask的run方法
    }


    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            inputCheck = true; //默认有效

//            if ((v.getId() == R.id.manager_function_button)
//                    || (v.getId() == R.id.manager_finger_button)
//                    || (v.getId() == R.id.manager_open_lock_button)) {
//                if (passwdState != true) {
//                    error_info.setText("请输入完整密码!");
//                    error_info.setVisibility(error_info.VISIBLE);
//                    return;
//                } else {
//                    if (v.getId() == R.id.manager_function_button) {
//                        Intent intent = new Intent();
//                        intent.setClass(ManagerActivity.this, Function_16_Activity.class);
//                        ManagerActivity.this.startActivity(intent);
//                        finish();
//                    } else if (v.getId() == R.id.manager_finger_button) {
//                        Intent intent = new Intent();
//                        intent.setClass(ManagerActivity.this, FingerActivity.class);
//                        ManagerActivity.this.startActivity(intent);
//                        finish();
//                    } else if (v.getId() == R.id.manager_open_lock_button) {
//                        Intent intent = new Intent();
//                        intent.setClass(ManagerActivity.this, Lock_16_Activity.class);
//                        ManagerActivity.this.startActivity(intent);
//                        finish();
//                    }
//                }
//            }

            Intent intent = new Intent();

            switch (v.getId()) {
                case R.id.manage_digit_00:
                    if(inputCnt < 6)
                        inputNum += "0";
                    break;
                case R.id.manage_digit_01:
                    if(inputCnt < 6)
                        inputNum += "1";
                    break;
                case R.id.manage_digit_02:
                    if(inputCnt < 6)
                        inputNum += "2";
                    break;
                case R.id.manage_digit_03:
                    if(inputCnt < 6)
                        inputNum += "3";
                    break;
                case R.id.manage_digit_04:
                    if(inputCnt < 6)
                        inputNum += "4";
                    break;
                case R.id.manage_digit_05:
                    if(inputCnt < 6)
                        inputNum += "5";
                    break;
                case R.id.manage_digit_06:
                    if(inputCnt < 6)
                        inputNum += "6";
                    break;
                case R.id.manage_digit_07:
                    if(inputCnt < 6)
                        inputNum += "7";
                    break;
                case R.id.manage_digit_08:
                    if(inputCnt < 6)
                        inputNum += "8";
                    break;
                case R.id.manage_digit_09:
                    if(inputCnt < 6)
                        inputNum += "9";
                    break;
                case R.id.manage_del_button:
                    inputCheck = false; //无效输入
                    passwdState = false;
                    if(inputNum.length() > 0) {
                        inputNum = inputNum.substring(0, inputNum.length() - 1);
                        inputCnt--;
                    }
                    break;
                case R.id.manage_ruturn:
                    inputCheck = false; //无效输入
                    //清理
                    passwdState = false;
                    inputCnt = 0;
                    exit = false;
                    finish();
                    break;
                case R.id.manager_function_button:
                        intent.setClass(ManagerActivity.this, Function_16_Activity.class);
                        ManagerActivity.this.startActivity(intent);
                    passwdState = false;
                        finish();
                    break;

                case R.id.manager_finger_button:
                        intent.setClass(ManagerActivity.this, FingerActivity.class);
                        ManagerActivity.this.startActivity(intent);
                    passwdState = false;
                        finish();
                    break;

                case R.id.manager_open_lock_button:
                        intent.setClass(ManagerActivity.this, Lock_16_Activity.class);
                        ManagerActivity.this.startActivity(intent);
                    passwdState = false;
                        finish();
                    break;
                default:
                    inputCheck = false; //无效输入
                    break;
            }

            if((inputCheck == true) && (inputCnt < 6))
            {
                inputCnt++;
            }

            //输入框显示*号
            input_1.setVisibility(View.INVISIBLE);
            input_2.setVisibility(View.INVISIBLE);
            input_3.setVisibility(View.INVISIBLE);
            input_4.setVisibility(View.INVISIBLE);
            input_5.setVisibility(View.INVISIBLE);
            input_6.setVisibility(View.INVISIBLE);
            switch(inputCnt){
                case 6:
                    input_6.setVisibility(View.VISIBLE);
                case 5:
                    input_5.setVisibility(View.VISIBLE);
                case 4:
                    input_4.setVisibility(View.VISIBLE);
                case 3:
                    input_3.setVisibility(View.VISIBLE);
                case 2:
                    input_2.setVisibility(View.VISIBLE);
                case 1:
                    input_1.setVisibility(View.VISIBLE);
                    break;
                default:
                    break;
            }

            if(inputCnt == 6){
                if (!inputNum.equals(g_passwd)) {
                    error_info.setText("密码错误!");
                    error_info.setVisibility(View.VISIBLE);
                }
                else{ //密码正确.
                    error_info.setText("密码正确!");
                    error_info.setVisibility(View.VISIBLE);
                    passwdState = true;

                    manage_finger.setClickable(true);
                    manage_open_lock.setClickable(true);
                    manage_function.setClickable(true);

                    manage_finger.setVisibility(View.VISIBLE);
                    manage_open_lock.setVisibility(View.VISIBLE);
                    manage_function.setVisibility(View.VISIBLE);

                    manage_finger_txt.setVisibility(View.VISIBLE);
                    manage_open_lock_txt.setVisibility(View.VISIBLE);
                    manage_function_txt.setVisibility(View.VISIBLE);
                }
            }
            else{
                error_info.setVisibility(View.INVISIBLE);

                manage_finger.setClickable(false);
                manage_open_lock.setClickable(false);
                manage_function.setClickable(false);

                manage_finger.setVisibility(View.INVISIBLE);
                manage_open_lock.setVisibility(View.INVISIBLE);
                manage_function.setVisibility(View.INVISIBLE);

                manage_finger_txt.setVisibility(View.INVISIBLE);
                manage_open_lock_txt.setVisibility(View.INVISIBLE);
                manage_function_txt.setVisibility(View.INVISIBLE);
            }

        }
    };
    // 将字符串写入到文本文件中
    public void writeTxtToFile(String strcontent, String filePath, String fileName) {

        int i, len;
        char charArry[] = new char[6];
        //生成文件夹之后，再生成文件，不然会出错
        makeFilePath(filePath, fileName);
        String strFilePath = filePath+fileName;
        StringBuilder strBuilder = new StringBuilder(strcontent);
        len = strcontent.length();
        charArry = strcontent.toCharArray();

        //简单加密,先小写后大写，放到ASCII后面看不到的数据就可以
        for(i = 0; i < 6; i++) {
            charArry[i] = (char)((int)charArry[i] - (int)'!');
            //charArry[i] = (char)((int)charArry[i] + (int)'!');
        }
        //strcontent = Arrays.toString(charArry);

        strcontent = String.valueOf(charArry);

        // 每次写入时，都换行写
        String strContent = strcontent + "\r\n";
        try {
            File file = new File(strFilePath);
            if (!file.exists()) {
                Log.d("TestFile", "Create the file:" + strFilePath);
                file.getParentFile().mkdirs();
                file.createNewFile();
            }

            RandomAccessFile raf = new RandomAccessFile(file, "rwd");
            //raf.seek(file.length());  //只字6位密码
            raf.write(strContent.getBytes());
            raf.close();
        } catch (Exception e) {
            Log.e("TestFile", "Error on write File:" + e);
        }
    }

    /**
     * 读入TXT文件
     */
    public void readFile(String filePath, String fileName) {
        int i, len;
        char charArry[] = new char[6];

        String pathname = filePath + fileName; // 绝对路径或相对路径都可以，写入文件时演示相对路径,
        //防止文件建立或读取失败，用catch捕捉错误并打印，也可以throw;
        //不关闭文件会导致资源的泄露，读写文件都同理
        //Java7的try-with-resources可以优雅关闭文件，异常时自动关闭文件；详细解读https://stackoverflow.com/a/12665271
        try {
            FileReader reader = new FileReader(pathname);
            BufferedReader br = new BufferedReader(reader); // 建立一个对象，它把文件内容转成计算机能读懂的语言

            String line;
            //网友推荐更加简洁的写法
            while ((line = br.readLine()) != null) {
                // 一次读入一行数据
                //System.out.println(line);
                //解密
                len = line.length();
                if(len != 6)
                {
                    Toast toast= Toast.makeText(ManagerActivity.this,"密码文件损坏！!",Toast.LENGTH_SHORT);
                    toast.show();
                    break;
                }
                charArry = line.toCharArray();

                //简单加密,先小写后大写，放到ASCII后面看不到的数据就可以
                for(i = 0; i < 6; i++) {
                    charArry[i] = (char)((int)charArry[i] + (int)'!');
                    //charArry[i] = (char)((int)charArry[i] + (int)'!');
                }
                g_passwd = String.valueOf(charArry);

//                g_passwd = line;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    // 生成文件
    public File makeFilePath(String filePath, String fileName) {
        File file = null;
        makeRootDirectory(filePath);
        try {
            file = new File(filePath + fileName);
            if (!file.exists()) {
                file.createNewFile();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return file;
    }

    // 生成文件夹
    public static void makeRootDirectory(String filePath) {
        File file = null;
        try {
            file = new File(filePath);
            if (!file.exists()) {
                file.mkdir();
            }
        } catch (Exception e) {
            Log.i("error:", e+"");
        }
    }

    /**
     * Android shell 命令执行器，支持无限个命令串型执行（需要有root权限！！）
     * <p>
     * <p>
     * HOW TO USE?
     * Example:修改开机启动动画。把/sdcard/Download目录下的bootanimation.zip文件拷贝到
     * /system/media目录下并修改bootanimation.zip的权限为777。
     * <p>
     * <pre>
     *      int result = new ShellCommandExecutor()
     *                  .addCommand("mount -o remount,rw /system")
     *                  .addCommand("cp /sdcard/Download/bootanimation.zip /system/media")
     *                  .addCommand("cd /system/media")
     *                  .addCommand("chmod 777 bootanimation.zip")
     *                  .execute();
     * <pre/>
     *
     * @author AveryZhong.
     */
    public class ShellCommandExecutor {
        private static final String TAG = "ShellCommandExecutor";

        private StringBuilder mCommands;

        public ShellCommandExecutor() {
            mCommands = new StringBuilder();
        }

        public int execute() {
            return execute(mCommands.toString());
        }

        public ManagerActivity.ShellCommandExecutor addCommand(String cmd) {
            if (TextUtils.isEmpty(cmd)) {
                throw new IllegalArgumentException("command can not be null.");
            }
            mCommands.append(cmd);
            mCommands.append("\n");
            return this;
        }

        private  int execute(String command) {
            int result = -1;
            DataOutputStream dos = null;
            try {
                Process p = Runtime.getRuntime().exec("su");
                dos = new DataOutputStream(p.getOutputStream());
                Log.i(TAG, command);
                dos.writeBytes(command + "\n");
                dos.flush();
                dos.writeBytes("exit\n");
                dos.flush();
                p.waitFor();
                result = p.exitValue();
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                if (dos != null) {
                    try {
                        dos.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
            return result;
        }
    }
}
