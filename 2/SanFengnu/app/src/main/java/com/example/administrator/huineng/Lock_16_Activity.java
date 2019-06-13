package com.example.administrator.huineng;

import android.annotation.SuppressLint;
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

import com.example.administrator.huineng.R;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Timer;
import java.util.TimerTask;

public class Lock_16_Activity extends AppCompatActivity {


    private  View glassInclude[] = new View[15];

    private  Button glass_return = null;
    private  Button glassOpenLock[] = new Button[15];
    private  Button glassTrunOn[] = new Button[15];
    private  Button glassBottomCicle[] = new Button[15];
    private  Button glassTopCicle[] = new Button[15];
    private ProgressBar process_bar[] = new  ProgressBar[15];

    protected  int glass_num;
    protected  int pressState[] = new int[15];   //

    protected  volatile int lock_timer[] = new int[15];   //开锁时间，最长，没有收到数据就默认已经关锁
    protected  volatile int trun_timer[] = new int[15];   //转动时间，最长，没有收到数据就默认已经关锁

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
    public static   String g_passwd = new String("");

    private com.example.x6.serial.SerialPort serialttyS0;  //使用的是S2
    private InputStream ttyS0InputStream;//使用的是S2
    private OutputStream ttyS0OutputStream;//使用的是S2
    /* 打开串口 */
    private void init_serial() {
        try {
            //使用的是S2

            serialttyS0 = new com.example.x6.serial.SerialPort(new File("/dev/ttyS2"),115200,0, 10);
//            serialttyS0 = new com.example.x6.serial.SerialPort(new File("/dev/ttyS2"),115200,0, 10);
            ttyS0InputStream = serialttyS0.getInputStream();
            ttyS0OutputStream = serialttyS0.getOutputStream();
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
//        terimalPackage[0] = (byte)0xAA;			   //包头
//        terimalPackage[1] = (byte)data[0];         //cmd 命令
//        terimalPackage[2] = (byte)data[1];         //参数0  地址，LED模式，增加删除指纹ID,
//        terimalPackage[3] = (byte)data[2];         //
//        terimalPackage[4] = (byte)data[3];         //
//        terimalPackage[5] = (byte)data[4];         //数据  （锁开关 + 方向 + 时间）
//        terimalPackage[6] = (byte)data[5];         //
//        terimalPackage[7] = (byte)data[6];         //
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
    private void sendReadyPack() {  //发送就绪包
        byte[] temp_bytes = new byte[]{0x03, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00};  // 03 reday 命令，0x11保留参数
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
    private int i= 0;
    private int index= 0;
    private int cnt= 0;
    private int sizeRec= 0;
    private byte[] glassData = new byte[50];    //一个完整数据包
    private byte checkSum = 0;
    private byte ackCheck= (byte)0xFF;  //一个字节的应答判断
    private byte zero_num= (byte)0;  //记录第几个手表进入到零点


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
        for(i = 0 ; i < 53; i++) //和校验
        {
            checkSum += recvArry[index + i];
        }

        if(checkSum != (byte)0x55 - 1)
            return -1; //

        //应答地址 0xFF
//        u8aSendArr[1] = MCU_ACK_ADDR;//应答地址
//        u8aSendArr[2] = ZERO_ACK_TYPE;       //应答类型零点
//        u8aSendArr[3] = u8GlassAddr;       //手表地址
        if(recvArry[index + 1] == (byte)0xff)        {
            if(recvArry[index + 2]  == (byte)0x02)   //0x01 零点应答类型
            {
                zero_num = recvArry[index + 3];
                ackCheck = 0;   //校验正确

                return 0;
            }
        }

        ackCheck = (byte) 0xFF;
        return 0;
    }

    volatile  private boolean exit = false;

    protected int checkRecPack() {   //串口接收数据

        int tmp_cnt = 0;

        try {
            while(exit != true) {
                sizeRec = 0;
                do {
                    cnt = -1;
                    if(ttyS0InputStream != null)
                        cnt = ttyS0InputStream.read(recvData);
                    if (cnt != -1) {
                        System.arraycopy(recvData, 0, recvArry, sizeRec, cnt);
                        sizeRec += cnt;
                    }
                } while ((sizeRec < 53) && (exit == false));  //少于一个包数据  //或者已经退出

                if (exit == true)
                    return -1;

                if (jungleRecPack() == 0) //检查参数合法性
                {
                    //if(zero_num ==  glass_num)
                        break;
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        return 0;
    }


    WorkThread myThread = new WorkThread();  //串口接收数据

    public class WorkThread extends Thread {
        @Override
        public void run() {
            super.run();
            /**
             耗时操作
             */
            while(!isInterrupted()) {
                if(checkRecPack() == 0) {
                    //由timer发送消息
                    //要判断是哪个锁到了零点
                    if(lock_timer[zero_num] > 0)
                        lock_timer[zero_num] = 2;  //2ms后发送
                    // if(trun_timer > 0) trun_timer = 2;
                }
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        int i;

        exit =  true;
        if (myThread != null) {
            Log.d("aaa", "lock_16_  发送线程停止interrupt。。。。。。");
            myThread.interrupt();
        }

        tick_task.cancel();
        serial_task.cancel();
        // 移除所有消息
        handler.removeCallbacksAndMessages(null);
        // 或者移除单条消息
        for(i = 0; i < 16; i++) {
            handler.removeMessages((0x80 | i));
            handler.removeMessages((0x40 | i));
        }

        //关闭串口
        try {
            ttyS0InputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        try {
            ttyS0OutputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (serialttyS0 != null) {
            serialttyS0.close();
            serialttyS0 = null;
        }
    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){

        byte msgNum = 0;
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            msgNum = (byte)msg.what;

            if((msgNum & (byte)(0x80)) != 0) {
                msgNum &= (byte)(0x7F);
                glassTrunOn[msgNum].setEnabled(true);
                glassOpenLock[msgNum].setVisibility(View.INVISIBLE);
                glassTrunOn[msgNum].setVisibility(View.VISIBLE);
                glassTopCicle[msgNum].setVisibility(View.INVISIBLE);
                glassBottomCicle[msgNum].setVisibility(View.VISIBLE);
                process_bar[msgNum].setVisibility(View.GONE);
            }else {
                msgNum &= (byte)(0x3F);
                glassOpenLock[msgNum].setEnabled(true);
                glassOpenLock[msgNum].setVisibility(View.VISIBLE);
                glassTrunOn[msgNum].setVisibility(View.INVISIBLE);
                glassTopCicle[msgNum].setVisibility(View.VISIBLE);
                glassBottomCicle[msgNum].setVisibility(View.INVISIBLE);
                process_bar[msgNum].setVisibility(View.GONE);
            }
        }
    };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomUIMenu();
        setContentView(R.layout.activity_lock_16_);

        for(i = 0; i < 15; i++)
        {
            pressState[i] = 1; //默认赋值关锁状态
        }
        
        initViews();
        init_serial();          //初始化串口

        Timer timer = new Timer();
        timer.schedule(tick_task, 1, 1);//1ms后执行Tick   1ms 的tick
        timer.schedule(serial_task, 50);//300ms后执行TimeTask的run方法
    }

    TimerTask serial_task = new TimerTask() {
        @Override
        public void run() {

            /**
             *要执行的操作
             */
            myThread.start();        /*延时一段时间再开启线程*/
        }
    };

    TimerTask tick_task = new TimerTask() {
        int i;

        @Override
        public void run() {
            /**
             *要执行的操作
             */
            for(i = 0; i < 15; i++){
                if(lock_timer[i] > 0){
                    lock_timer[i]--;
                    if(lock_timer[i] == 0)
                    {
                        Message msg = Message.obtain();
                        handler.sendEmptyMessageDelayed((i | 0x80), 1);  //发送消息
                    }
                }

                if(trun_timer[i] > 0){
                    trun_timer[i]--;
                    if(trun_timer[i] == 0)
                    {
                        Message msg = Message.obtain();
                        handler.sendEmptyMessageDelayed((i | 0x40), 1);  //发送消息
                    }
                }
            }
        }
    };
    private void initViews() {
        //这是获得include中的控件
        glassInclude[0] = findViewById(R.id.lock16_glass_01);
        glassInclude[1] = findViewById(R.id.lock16_glass_02);
        glassInclude[2] = findViewById(R.id.lock16_glass_03);
        glassInclude[3] = findViewById(R.id.lock16_glass_04);
        glassInclude[4] = findViewById(R.id.lock16_glass_05);
        glassInclude[5] = findViewById(R.id.lock16_glass_06);
        glassInclude[6] = findViewById(R.id.lock16_glass_07);
        glassInclude[7] = findViewById(R.id.lock16_glass_08);
        glassInclude[8] = findViewById(R.id.lock16_glass_09);
        glassInclude[9] = findViewById(R.id.lock16_glass_10);
        glassInclude[10] = findViewById(R.id.lock16_glass_11);
        glassInclude[11] = findViewById(R.id.lock16_glass_12);
        glassInclude[12] = findViewById(R.id.lock16_glass_13);
        glassInclude[13] = findViewById(R.id.lock16_glass_14);
        glassInclude[14] = findViewById(R.id.lock16_glass_15);
        //获取include中的子控件
        glassOpenLock[0] = glassInclude[0] .findViewById(R.id.include_open_lock_button);
        glassOpenLock[1] = glassInclude[1] .findViewById(R.id.include_open_lock_button);
        glassOpenLock[2] = glassInclude[2] .findViewById(R.id.include_open_lock_button);
        glassOpenLock[3] = glassInclude[3] .findViewById(R.id.include_open_lock_button);
        glassOpenLock[4] = glassInclude[4] .findViewById(R.id.include_open_lock_button);
        glassOpenLock[5] = glassInclude[5] .findViewById(R.id.include_open_lock_button);
        glassOpenLock[6] = glassInclude[6] .findViewById(R.id.include_open_lock_button);
        glassOpenLock[7] = glassInclude[7] .findViewById(R.id.include_open_lock_button);
        glassOpenLock[8] = glassInclude[8] .findViewById(R.id.include_open_lock_button);
        glassOpenLock[9] = glassInclude[9] .findViewById(R.id.include_open_lock_button);
        glassOpenLock[10] = glassInclude[10].findViewById(R.id.include_open_lock_button);
        glassOpenLock[11] = glassInclude[11].findViewById(R.id.include_open_lock_button);
        glassOpenLock[12] = glassInclude[12].findViewById(R.id.include_open_lock_button);
        glassOpenLock[13] = glassInclude[13].findViewById(R.id.include_open_lock_button);
        glassOpenLock[14] = glassInclude[14].findViewById(R.id.include_open_lock_button);

        glassTrunOn[0]  = glassInclude[0] .findViewById(R.id.include_trun_moto_button);
        glassTrunOn[1]  = glassInclude[1] .findViewById(R.id.include_trun_moto_button);
        glassTrunOn[2]  = glassInclude[2] .findViewById(R.id.include_trun_moto_button);
        glassTrunOn[3]  = glassInclude[3] .findViewById(R.id.include_trun_moto_button);
        glassTrunOn[4]  = glassInclude[4] .findViewById(R.id.include_trun_moto_button);
        glassTrunOn[5]  = glassInclude[5] .findViewById(R.id.include_trun_moto_button);
        glassTrunOn[6]  = glassInclude[6] .findViewById(R.id.include_trun_moto_button);
        glassTrunOn[7]  = glassInclude[7] .findViewById(R.id.include_trun_moto_button);
        glassTrunOn[8]  = glassInclude[8] .findViewById(R.id.include_trun_moto_button);
        glassTrunOn[9]  = glassInclude[9] .findViewById(R.id.include_trun_moto_button);
        glassTrunOn[10] = glassInclude[10].findViewById(R.id.include_trun_moto_button);
        glassTrunOn[11] = glassInclude[11].findViewById(R.id.include_trun_moto_button);
        glassTrunOn[12] = glassInclude[12].findViewById(R.id.include_trun_moto_button);
        glassTrunOn[13] = glassInclude[13].findViewById(R.id.include_trun_moto_button);
        glassTrunOn[14] = glassInclude[14].findViewById(R.id.include_trun_moto_button);

        glassTopCicle[0]  = glassInclude[0] .findViewById(R.id.include_top_circle);
        glassTopCicle[1]  = glassInclude[1] .findViewById(R.id.include_top_circle);
        glassTopCicle[2]  = glassInclude[2] .findViewById(R.id.include_top_circle);
        glassTopCicle[3]  = glassInclude[3] .findViewById(R.id.include_top_circle);
        glassTopCicle[4]  = glassInclude[4] .findViewById(R.id.include_top_circle);
        glassTopCicle[5]  = glassInclude[5] .findViewById(R.id.include_top_circle);
        glassTopCicle[6]  = glassInclude[6] .findViewById(R.id.include_top_circle);
        glassTopCicle[7]  = glassInclude[7] .findViewById(R.id.include_top_circle);
        glassTopCicle[8]  = glassInclude[8] .findViewById(R.id.include_top_circle);
        glassTopCicle[9]  = glassInclude[9] .findViewById(R.id.include_top_circle);
        glassTopCicle[10] = glassInclude[10].findViewById(R.id.include_top_circle);
        glassTopCicle[11] = glassInclude[11].findViewById(R.id.include_top_circle);
        glassTopCicle[12] = glassInclude[12].findViewById(R.id.include_top_circle);
        glassTopCicle[13] = glassInclude[13].findViewById(R.id.include_top_circle);
        glassTopCicle[14] = glassInclude[14].findViewById(R.id.include_top_circle);

        glassBottomCicle[0]  = glassInclude[0] .findViewById(R.id.include_bottom_circle);
        glassBottomCicle[1]  = glassInclude[1] .findViewById(R.id.include_bottom_circle);
        glassBottomCicle[2]  = glassInclude[2] .findViewById(R.id.include_bottom_circle);
        glassBottomCicle[3]  = glassInclude[3] .findViewById(R.id.include_bottom_circle);
        glassBottomCicle[4]  = glassInclude[4] .findViewById(R.id.include_bottom_circle);
        glassBottomCicle[5]  = glassInclude[5] .findViewById(R.id.include_bottom_circle);
        glassBottomCicle[6]  = glassInclude[6] .findViewById(R.id.include_bottom_circle);
        glassBottomCicle[7]  = glassInclude[7] .findViewById(R.id.include_bottom_circle);
        glassBottomCicle[8]  = glassInclude[8] .findViewById(R.id.include_bottom_circle);
        glassBottomCicle[9]  = glassInclude[9] .findViewById(R.id.include_bottom_circle);
        glassBottomCicle[10] = glassInclude[10].findViewById(R.id.include_bottom_circle);
        glassBottomCicle[11] = glassInclude[11].findViewById(R.id.include_bottom_circle);
        glassBottomCicle[12] = glassInclude[12].findViewById(R.id.include_bottom_circle);
        glassBottomCicle[13] = glassInclude[13].findViewById(R.id.include_bottom_circle);
        glassBottomCicle[14] = glassInclude[14].findViewById(R.id.include_bottom_circle);
        glass_return = findViewById(R.id.lock_16_return);


        process_bar[0] = findViewById(R.id.lock16_process_bar_01);
        process_bar[1] = findViewById(R.id.lock16_process_bar_02);
        process_bar[2] = findViewById(R.id.lock16_process_bar_03);
        process_bar[3] = findViewById(R.id.lock16_process_bar_04);
        process_bar[4] = findViewById(R.id.lock16_process_bar_05);
        process_bar[5] = findViewById(R.id.lock16_process_bar_06);
        process_bar[6] = findViewById(R.id.lock16_process_bar_07);
        process_bar[7] = findViewById(R.id.lock16_process_bar_08);
        process_bar[8] = findViewById(R.id.lock16_process_bar_09);
        process_bar[9] = findViewById(R.id.lock16_process_bar_10);
        process_bar[10] = findViewById(R.id.lock16_process_bar_11);
        process_bar[11] = findViewById(R.id.lock16_process_bar_12);
        process_bar[12] = findViewById(R.id.lock16_process_bar_13);
        process_bar[13] = findViewById(R.id.lock16_process_bar_14);
        process_bar[14] = findViewById(R.id.lock16_process_bar_15);

                // 3.设置按钮点击事件
        glass_return.setOnClickListener(onClickListener);
        glassOpenLock[0] .setOnClickListener(onClickListener);
        glassOpenLock[1] .setOnClickListener(onClickListener);
        glassOpenLock[2] .setOnClickListener(onClickListener);
        glassOpenLock[3] .setOnClickListener(onClickListener);
        glassOpenLock[4] .setOnClickListener(onClickListener);
        glassOpenLock[5] .setOnClickListener(onClickListener);
        glassOpenLock[6] .setOnClickListener(onClickListener);
        glassOpenLock[7] .setOnClickListener(onClickListener);
        glassOpenLock[8] .setOnClickListener(onClickListener);
        glassOpenLock[9] .setOnClickListener(onClickListener);
        glassOpenLock[10].setOnClickListener(onClickListener);
        glassOpenLock[11].setOnClickListener(onClickListener);
        glassOpenLock[12].setOnClickListener(onClickListener);
        glassOpenLock[13].setOnClickListener(onClickListener);
        glassOpenLock[14].setOnClickListener(onClickListener);

        glassTrunOn[0] .setOnClickListener(onClickListener);
        glassTrunOn[1] .setOnClickListener(onClickListener);
        glassTrunOn[2] .setOnClickListener(onClickListener);
        glassTrunOn[3] .setOnClickListener(onClickListener);
        glassTrunOn[4] .setOnClickListener(onClickListener);
        glassTrunOn[5] .setOnClickListener(onClickListener);
        glassTrunOn[6] .setOnClickListener(onClickListener);
        glassTrunOn[7] .setOnClickListener(onClickListener);
        glassTrunOn[8] .setOnClickListener(onClickListener);
        glassTrunOn[9] .setOnClickListener(onClickListener);
        glassTrunOn[10].setOnClickListener(onClickListener);
        glassTrunOn[11].setOnClickListener(onClickListener);
        glassTrunOn[12].setOnClickListener(onClickListener);
        glassTrunOn[13].setOnClickListener(onClickListener);
        glassTrunOn[14].setOnClickListener(onClickListener);
    }


    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            byte[] temp_bytes = new byte[]{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // 0x02 更新状状态命令
            boolean inputCheck = true; //默认有效
            byte keyNum = 0; //按键号

            final int TRUN_TIME = 1000;
            final int OPEN_LOCK_TIME = 6000;
            

            if (v.getId() == R.id.lock_16_return) {    //把返回键
                inputCheck = false;

                //清理资源
                //关闭串口
                serialttyS0.close();
                try {
                    ttyS0InputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                try {
                    ttyS0OutputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                finish();
            }
            /*******************开锁按键处理**************************/
            //先取入include 布件
            //获得父控件的对象，然后获得父控件的id
            ViewGroup parent = (ViewGroup) v.getParent();
            switch (parent.getId()) {
                case R.id.lock16_glass_01:
                    keyNum = 0;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下    parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;

                case R.id.lock16_glass_02:
                    keyNum = 1;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;

                case R.id.lock16_glass_03:
                    keyNum = 2;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_04:
                    keyNum = 3;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_05:
                    keyNum = 4;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_06:
                    keyNum = 5;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_07:
                    keyNum = 6;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_08:
                    keyNum = 7;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_09:
                    keyNum = 8;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_10:
                    keyNum = 9;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_11:
                    keyNum = 10;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_12:
                    keyNum = 11;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_13:
                    keyNum = 12;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_14:
                    keyNum = 13;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                case R.id.lock16_glass_15:
                    keyNum = 14;
                    if (v.getId() == R.id.include_open_lock_button) {
                        if (pressState[keyNum] == 1) {
                            pressState[keyNum] = 0;//1表示开锁已经按下                        parent.include_open_lock_button.setEnabled(false); //禁止按下
                            lock_timer[keyNum] = OPEN_LOCK_TIME;
                        }
                    } else {
                        if (pressState[keyNum] == 0) {
                            pressState[keyNum] = 1;  //1表示转动已经按下
                            trun_timer[keyNum] = TRUN_TIME;
                        }
                    }
                    break;
                default:
                    break;
            }

            if(inputCheck){
                glassOpenLock[keyNum].setEnabled(false); //禁止按下
                glassTrunOn[keyNum].setEnabled(false); //禁止按下
                process_bar[keyNum].setVisibility(View.VISIBLE);
                glassOpenLock[keyNum].setVisibility(View.VISIBLE);
                glassTrunOn[keyNum].setVisibility(View.VISIBLE);
                glassTopCicle[keyNum].setVisibility(View.INVISIBLE);
                glassBottomCicle[keyNum].setVisibility(View.INVISIBLE);
                
                temp_bytes[0] = (byte) 0x06;       //0x06 开锁命令
                temp_bytes[1] = keyNum; //地址 参数0  地址，LED模式，增加删除指纹ID,
                temp_bytes[4] = (byte) pressState[keyNum];//锁开关
                byte[] send = makeStringtoFramePackage(temp_bytes);
                /*串口发送字节*/
                try {
                    ttyS0OutputStream.write(send);
                    //ttyS1InputStream.read(send);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    };

}
