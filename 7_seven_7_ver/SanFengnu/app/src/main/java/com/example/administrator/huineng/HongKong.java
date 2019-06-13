package com.example.administrator.huineng;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.constraint.ConstraintLayout;
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
                finish();
            }else if(msgNum == 0x3300){
                tryWear.setVisibility(View.VISIBLE);
                bu_return.setVisibility(View.VISIBLE);
            }
        }
    };

    private com.example.x6.serial.SerialPort serialttyS0;  //使用的是S2
    private InputStream ttyS0InputStream;//使用的是S2
    private OutputStream ttyS0OutputStream;//使用的是S2
    /* 打开串口 */
    private void init_serial() {
        try {
            //使用的是S2

            serialttyS0 = new com.example.x6.serial.SerialPort(new File("/dev/ttyS1"),115200,0, 10);
//            serialttyS0 = new com.example.x6.serial.SerialPort(new File("/dev/ttyS1"),115200,0, 10);
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

//        #define     FINGER_ACK_TYPE         0x01  //指纹应答地址
//#define     ZERO_ACK_TYPE           0x02  //零点应答地址
//#define     DETECT_ACK_TYPE         0x03  //传感器应答
//#define     ERROR_ACK_TYPE          0xFF  //其他应答
        if(recvArry[index + 1] == (byte)0xff)        {
            if(recvArry[index + 2]  == (byte)0x03)   //0x02 零点应答类型
            {
                zero_num = recvArry[index + 3];
                ackCheck = 0;   //校验正确
                return ackCheck;
            }
        }

        ackCheck = (byte) 0xFF;
        return ackCheck;
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
                        break;
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        return 0;
    }

    SendWorkThread sendThread = new SendWorkThread();  //串口接收数据

    public class SendWorkThread extends Thread {

        byte[] temp_bytes = new byte[]{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // 0x02 更新状状态命令


        @Override
        public void run() {
            super.run();
            /**
             耗时操作
             */
            temp_bytes[0] = (byte) 0x0B;       //0x0B  切换锁=命令
            temp_bytes[1] = (byte) glass_num; //地址 参数0  地址，LED模式，增加删除指纹ID,
            temp_bytes[4] = (byte) 0;//锁开关

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

    TimerTask swnd_task = new TimerTask() {
        @Override
        public void run() {

            /**
             *要执行的操作
             */
            sendThread.start();        /*延时一段时间再开启线程*/
            this.cancel();
        }
    };

    WorkThread myThread = new WorkThread();  //串口接收数据

    public class WorkThread extends Thread {
        @Override
        public void run() {
            super.run();
            /**
             耗时操作
             */
            while(!isInterrupted()) {
                if(checkRecPack() == 0) {  //接收到串口发送过来的信息，是已经上升完成的状态
                    bHaveSent = false;
                    timer_cnt = 10;  //接收到finish准备就绪包
                }
            }
        }
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

    private  volatile int timer_cnt = 10000; //换一个锁最大的时间大概是12S
    private volatile  boolean bHaveSent = false;
    TimerTask tick_task = new TimerTask() {
        @Override
        public void run() {
            /**
             *要执行的操作
             */
            Message msg = Message.obtain();
            if(timer_cnt > 0) //默认这里的开锁 一发过去 就是开锁成功的了， 关锁也一样，没有检测到零点才开的机制。这个没有零点检测。
            {
                timer_cnt--;
            }else if(!bHaveSent){
                bHaveSent = true;
                handler.sendEmptyMessageDelayed((0x3300), 1);  //发送消息
            }
        }
    };

    @Override
    protected void onDestroy() {
        super.onDestroy();
        exit =  true;
        if (myThread != null) {
            Log.d("aaa", "HONGKONG  发送线程停止interrupt。。。。。。");
            myThread.interrupt();
        }

        tick_task.cancel();
        serial_task.cancel();
        // 移除所有消息
        handler.removeCallbacksAndMessages(null);
        // 或者移除单条消息
        handler.removeMessages(0x2200);
        handler.removeMessages(0x3300);

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

    volatile  private int glass_num = 0;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomUIMenu();

        Intent intent = getIntent();
        Bundle bundle = this.getIntent().getExtras();
        glass_num = bundle.getInt("glass_num");

        setContentView(R.layout.activity_hong_kong);

        init_serial();          //初始化串口
        initViews();

        Timer timer = new Timer();
        timer.schedule(swnd_task, 300);//300ms后执行开锁命令
        timer.schedule(tick_task, 1, 1);//1ms后执行Tick   1ms 的tick
        timer.schedule(serial_task, 360);//300ms后执行TimeTask的run方法
    }

    private void initViews() {

//        String myJpgPath =  "/mnt/sdcard/SanFeng/image/h1.png";
        ImageView jpgView= findViewById(R.id.hongkong_watchinfo);
//        switch (glass_num)
//        {
//            case 0:
//                myJpgPath = "/mnt/sdcard/SanFeng/image/h1.png";
//                break;
//            case 1:
//                myJpgPath = "/mnt/sdcard/SanFeng/image/h2.png";
//                break;
//            case 2:
//                myJpgPath = "/mnt/sdcard/SanFeng/image/h3.png";
//                break;
//            case 3:
//                myJpgPath = "/mnt/sdcard/SanFeng/image/h4.png";
//                break;
//            default:
//                break;
//        }
//
//        BitmapFactory.Options options = new BitmapFactory.Options();
//        Bitmap bm = BitmapFactory.decodeFile(myJpgPath, null);
//        jpgView.setImageBitmap(bm);

        switch(glass_num)
        {
            case 0:
                jpgView.setImageDrawable(getResources().getDrawable(R.drawable.hongkong_select_a));
                break;
            case 1:
//                ConstraintLayout.LayoutParams layoutParams = (ConstraintLayout.LayoutParams) jpgView.getLayoutParams();
//                layoutParams.leftMargin = 94;
//                jpgView.setLayoutParams(layoutParams);

                jpgView.setImageDrawable(getResources().getDrawable(R.drawable.hongkong_select_b));
                break;
            case 2:
                jpgView.setImageDrawable(getResources().getDrawable(R.drawable.hongkong_select_c));
                break;
            case 3:
                jpgView.setImageDrawable(getResources().getDrawable(R.drawable.hongkong_select_d));
                break;
            default:
                jpgView.setImageDrawable(getResources().getDrawable(R.drawable.hongkong_select_a));
                break;
        }

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
