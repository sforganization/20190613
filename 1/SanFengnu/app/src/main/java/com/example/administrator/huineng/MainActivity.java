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
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.example.administrator.huineng.R;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends AppCompatActivity {

    //        typedef enum
//        {
//                    MOTO_TIME_TPD = 0, //每天的动作模式，工作12小时，停止12小时
//                    MOTO_TIME_650, //旋转2分钟，停止942S
//                    MOTO_TIME_750,  //旋转2分钟，停止800S
//                    MOTO_TIME_850,  //旋转2分钟，停止693S
//                    MOTO_TIME_1000, //旋转2分钟，停止570S
//                    MOTO_TIME_1950, //旋转2分钟，停止234S
//        }MotoTime_e;
//        typedef enum
//        {
//                    MOTO_FR_FWD = 0,  //正转
//                    MOTO_FR_REV,      //反转
//                    MOTO_FR_FWD_REV, //正反转
//                    MOTO_FR_STOP, //停止
//        }MotoFR;


    protected  final  byte MOTO_TIME_TPD    = 0;
    protected  final  byte MOTO_TIME_650    = 1;
    protected  final  byte MOTO_TIME_750    = 2;
    protected  final  byte MOTO_TIME_850    = 3;
    protected  final  byte MOTO_TIME_1000   = 4;
    protected  final  byte MOTO_TIME_1950   = 5;

    protected  final  byte MOTO_FR_FWD      = 0;
    protected  final  byte MOTO_FR_REV      = 1;
    protected  final  byte MOTO_FR_FWD_REV  = 2;

    private Button enter_button = null;
    private ProgressBar bar = null;
    private TextView text_view = null;
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

    private com.example.x6.serial.SerialPort serialttyS0;
    private InputStream ttyS0InputStream;
    private OutputStream ttyS0OutputStream;
    /* 打开串口 */
    private void init_serial() {
        try {
            serialttyS0 = new com.example.x6.serial.SerialPort(new File("/dev/ttyS2"),115200,0,10);
            //serialttyS0 = new com.example.x6.serial.SerialPort(new File("/dev/ttyS0"),115200,0,10);
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

    private byte[] recvData = new byte[1024];    //每次最大是32位
    private byte[] recvArry = new byte[256];    //设置缓存大概4级    256/ 53 ~ 4
    private int i= 0;
    private int index= 0;
    private int cnt= 0;
    private int sizeRec= 0;
    private byte[] glassData = new byte[50];    //一个完整数据包
    private byte[] glassDataWrite = new byte[50];    //一个完整数据包
    private byte checkSum = 0;
    volatile  private boolean exit = false;

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

        //封包
        //public static void arraycopy(Object src, int srcPos, Object dest, int destPos, int length)
        System.arraycopy(recvArry, index + 1, glassData, 0, 50);
        return 0;
    }

    protected void checkStateInval() {  //检测数据是否全部合法
        int i;
        byte byteTem;

        //地址 + 命令 + 数据包[48] （锁开关 + 方向 + 时间）
        //这里锁开关不判断，默认转起来会关



        for(i = 0; i < 16; i++){   //纠正写错的
            if((glassData[2 + (i * 3) + 1] < MOTO_FR_FWD) || (glassData[2 + (i * 3) + 1] > MOTO_FR_FWD_REV)) {
                glassData[2 + (i * 3) + 1] = MOTO_FR_FWD;
            }

            if ((glassData[2 + (i * 3) + 2] < MOTO_TIME_TPD) || (glassData[2 + (i * 3) + 2] > MOTO_TIME_1950)) {
                glassData[2 + (i * 3) + 2] = MOTO_TIME_650;
            }
        }

        //数据正确，写入文件
        String filePath = "/mnt/sdcard/SanFeng/data/";
        String fileName = "stateInfo";   //保存用户信息，哪个指纹已经录入系统
        glassDataWrite = glassData.clone();
        glassDataWrite[0] = (byte) (glassDataWrite[0] ^ (byte) 0x80);  //去除最高位
        String str = new String(glassDataWrite);
        writeTxtToFile(str, filePath, fileName);
//            readFile(filePath, fileName);


    }

    protected void checkRecPack() {   //串口接收数据


          int tmp_cnt = 0;
          boolean checkResult = false;
        if( exit == true)
            return;

        try {
            for(tmp_cnt = 0; tmp_cnt < 5; tmp_cnt++) //连续发5次，不行就是认为单片机已经坏，或者接线已经松开
            {
                sizeRec = 0;
                do {
                    cnt = -1;
                    if(ttyS0InputStream != null)
                        cnt = ttyS0InputStream.read(recvData);//非阻塞读取，超时返回-1
                    //非阻塞读取，超时返回-1
                    if(cnt != -1) {
                        if(cnt >= 128) {
                            Log.d("fff", "主界面，一次接收到数据超过128字节。。。。。。");
                        }else {
                            if ((sizeRec + cnt) <= 128){
                            System.arraycopy(recvData, 0, recvArry, sizeRec, cnt);
                            sizeRec += cnt;
                            }else{
                                Log.d("fff", "主界面，一次接收到数据超过128字节。。。。。。");
                            }
                        }
                    }
                } while ((sizeRec < 53) && (exit == false));  //少于一个包数据  //或者已经退出


                if( exit == true)
                    return;

                if(jungleRecPack() == 0) //检查参数合法性
                {
                    Log.d("fff", "主界面，接收到正确数据。。。。。。");
                    //检测状态合法性，然后写入文件
                    checkStateInval();
                    checkResult = true;
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
                            sendReadyPack();        /*串口重新发送就绪包*/
                            this.cancel();
                        }
                    };
                    Timer timer = new Timer();
                    timer.schedule(task, 200);//300ms后执行TimeTask的run方法
                }
                Log.d("fff", "onCreate: dd");
            }
        } catch (IOException e) {
            i = 5;
            e.printStackTrace();
        }


        //数据校验成功则关串口，
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

        Log.d("fff", "中断标志外面关串口。。。。。。");  //关闭串口
        //关闭串口
        if (serialttyS0 != null) {
            serialttyS0.close();
            Log.d("fff", "main 串口 close。。。。。。");  //关闭串口
            serialttyS0 = null;
        }

        exit = true;   //退出不再发送
        if((tmp_cnt < 5) && (checkResult == true)) {
                //从全局池中返回一个message实例，避免多次创建message（如new Message）
                Message msg = Message.obtain();
//            msg.obj = glassData;
//            msg.what=1;   //标志消息的标志
                handler.sendEmptyMessageDelayed(1, 2);  //发送消息
        }
        else {
//            Looper.prepare();   //只能在主线程中使用toast 所以要加这两句
//            Toast toast= Toast.makeText(MainActivity.this,"CONNETCT FAILED!",Toast.LENGTH_SHORT);
//            toast.show();
//            Looper.loop();
        }
    }


    //    @SuppressLint("HandlerLeak")
    @Override
    protected void onDestroy() { //Activity退出时会调用
        super.onDestroy();
        exit = true;
        if (myThread != null) {
            Log.d("fff", "destory 主界面 interrupt。。。。。。");
            myThread.interrupt();
        }

        // 移除所有消息
        handler.removeCallbacksAndMessages(null);
        // 或者移除单条消息
        handler.removeMessages(1);
    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {      //判断标志位
                case 1:
                    /**
                     获取数据，更新UI
                     */
                    exit = true;
                    Intent intent = new Intent();
                    Bundle myBudle=new Bundle();
                    myBudle.putByteArray("glassData", glassData);
                    intent.putExtras(myBudle);


//                    // 移除所有消息
//                    handler.removeCallbacksAndMessages(null);
//                    // 或者移除单条消息
//                    handler.removeMessages(msg.what);

                    if((glassData[1] & 0x7F) == 16)   //6个手表
                    {
                        Toast toast= Toast.makeText(MainActivity.this,"3个!",Toast.LENGTH_SHORT);
                        toast.show();
                        Log.d("fff", "main 由事件进入");  //
                        intent.setClass(MainActivity.this, HongKong.class);
                        MainActivity.this.startActivity(intent);
                        finish();
                    }
                    else
                    {
                        //Toast toast= Toast.makeText(MainActivity.this,"初始化完成！",Toast.LENGTH_SHORT);
                        //toast.show();
                        Log.d("fff", "main 由事件进入。。。。。。");  //
                        intent.setClass(MainActivity.this, HongKong.class);
                        MainActivity.this.startActivity(intent);
                        finish();
                    }
                    break;
                default:
                    break;
            }
        }
    };

    WorkThread myThread = new WorkThread();  //串口发送数据
    public class WorkThread extends Thread {

        @Override
        public void run() {
            super.run();
            /**
             耗时操作
             */

            sendReadyPack();        /*串口发送就绪包*/
            while (!isInterrupted()) {
                checkRecPack();
            }
        }
    }
    Timer timer = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomUIMenu();

        exit = false;
        setContentView(R.layout.activity_main);

        //新建一个File，传入文件夹目录
        String filePath = "/mnt/sdcard/SanFeng/";
        makeRootDirectory(filePath);
        filePath = "/mnt/sdcard/SanFeng/data/";
        makeRootDirectory(filePath);
        filePath = "/mnt/sdcard/SanFeng/image/";
        makeRootDirectory(filePath);
        filePath = "/mnt/sdcard/SanFeng/cleanup/";
        makeRootDirectory(filePath);
        filePath = "/mnt/sdcard/SanFeng/movies/";
        makeRootDirectory(filePath);

        //串口检测正确就写入文件中

//        enter_button = findViewById(R.id.enter_button);
        text_view    = findViewById(R.id.main_textview);

//      main_textview  enter_button.setOnClickListener(new View.OnClickListener(){
//            public void onClick(View v){
//                Intent intent = new Intent();
//                Bundle myBudle=new Bundle();
//                myBudle.putByteArray("glassData", glassData);
//                intent.putExtras(myBudle);
//                try {
//                    ttyS0InputStream.close();
//                } catch (IOException e) {
//                    e.printStackTrace();
//                }
//
//                try {
//                    ttyS0OutputStream.close();
//                } catch (IOException e) {
//                    e.printStackTrace();
//                }
//
//                Log.d("fff", "中断标志外面关串口。。。。。。");  //关闭串口
//                //关闭串口
//                if (serialttyS0 != null) {
//                    serialttyS0.close();
//                    Log.d("fff", "main 串口 close。。。。。。");  //关闭串口
//                    serialttyS0 = null;
//                }
//
//                Toast toast= Toast.makeText(MainActivity.this,"按键进入!",Toast.LENGTH_SHORT);
//                toast.show();
//                Log.d("fff", "main 按键进入。。。。。。");  //
//                intent.setClass(MainActivity.this, HongKong.class);
//                startActivity(intent);
//                finish();
//            }
//        });

        bar = findViewById(R.id.progressSelf);
        int i = 0;


//        init_serial();          //初始化串口
        timer = new Timer();
//        timer.schedule(serial_task, 2500);//300ms后执行TimeTask的run方法
        timer.schedule(loadingui_task, 1, 80);//1ms后执行Tick   50ms频率，到8秒为止
    }

    TimerTask serial_task = new TimerTask() {
        @Override
        public void run() {
            /**
             *要执行的操作
             */
            myThread.start();
        }
    };


     private  int timer_cnt = 0;
    TimerTask loadingui_task = new TimerTask() {
        @Override
        public void run() {

            /**
             *要执行的操作
             */
            if(timer_cnt < 30)
            {
                timer_cnt++;
                bar.setProgress(timer_cnt * 3);
            }else
            {
                Message msg = Message.obtain();
//            msg.obj = glassData;
//            msg.what=1;   //标志消息的标志
                timer.cancel();
                handler.sendEmptyMessageDelayed(1, 2);  //发送消息


//                runOnUiThread(new Runnable() {
//                    @Override
//                    public void run() {
////                        text_view.setText("系统初始化失败！！！");
////                        enter_button.setVisibility(View.VISIBLE);
////                        enter_button.setClickable(true);
//                    }
//                });
            }


        }
    };

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

    // 将字符串写入到文本文件中
    public void writeTxtToFile(String strcontent, String filePath, String fileName) {

        int i, len;
        char charArry[] = new char[6];
        //生成文件夹之后，再生成文件，不然会出错
        makeFilePath(filePath, fileName);
        String strFilePath = filePath+fileName;
        StringBuilder strBuilder = new StringBuilder(strcontent);
        len = strcontent.length();
//        charArry = strcontent.toCharArray();

        //简单加密,先小写后大写，放到ASCII后面看不到的数据就可以
//        for(i = 0; i < 6; i++) {
//            charArry[i] = (char)((int)charArry[i] - (int)'!');
//            //charArry[i] = (char)((int)charArry[i] + (int)'!');
//        }
        //strcontent = Arrays.toString(charArry);

//        strcontent = String.valueOf(charArry);

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
            Log.d("TestFile", "Error on write File:" + e);
        }
    }

    /**
     * 读入TXT文件
     */
    public int readFile(String filePath, String fileName) {
        int i, len;
        char charArry[] = new char[60];
        String str = new String();
        byte[] tem = new byte[50];    //一个完整数据包
        StringBuilder sb = new StringBuilder();
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
                len = line.length();   //指纹信息，0 未录入 1 已经录入
                str = line;
                tem = str.getBytes();
               //由于char是16位宽与string一致，所以如果用byte是1开头的会多占一个字节长度
                charArry = line.toCharArray();

                for(i = 0; i < 50; i++){
                    tem[i] = (byte)charArry[i];
                }
                Log.d("fff", "main haha");
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return 0;
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
}
