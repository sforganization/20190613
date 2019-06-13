package com.example.administrator.huineng;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Color;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
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

public class FingerActivity extends AppCompatActivity {

    protected final byte  FINGER_STATE_ADD =  1;
    protected final byte  FINGER_STATE_DEL =  2;
    protected final byte  FINGER_STATE_DEF =  (byte)0xff;

    protected final byte  ACK_SUCCESS       =  0;
    protected final byte  ACK_FAILED        =  1;
    protected final byte  ACK_AGAIN         =  2;

    protected final byte  MSG_SUCCESS       =  7;
    protected final byte  MSG_FAILED        =  8;
    protected final byte  MSG_AGAIN         =  9;
    protected final byte  MSG_INVISIABLE   =  10;  //隐藏TEXTVIEW

    String filePath = "/mnt/sdcard/SanFeng/data/";
    String fileName = "userInfo";   //保存用户信息，哪个指纹已经录入系统

    protected  int userID = 0;   //用户ID
    protected  int userIDSave = 0;   //保存前面一次选择的用户ID
    protected  volatile  int fingerState = 0;   //当前状态，是增加用户还是删除用户

    protected  String stringSave = new String();  //保存在文件中的字符串
    char userInfo[] = new char[6];

    protected  Button fingerReturn = null;
    protected  Button changPasswdReturn = null;
    protected  Button fingerDelUser = null;
    protected  Button fingerAddUser = null;
    protected  Button fingerID[]  = new Button[6];
    protected  TextView fingerTextView = null;

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

    private void sendAddUserPack() {  //发送包
        byte[] temp_bytes = new byte[]{0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // 0x04 增加指纹命令
        temp_bytes[1] = (byte)userID; //需要增加的USER   id

        byte[] send = makeStringtoFramePackage(temp_bytes);
        /*串口发送字节*/
        try {
            ttyS0OutputStream.write(send);
            //ttyS1InputStream.read(send);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void sendDelUserPack() {  //发送包
        byte[] temp_bytes = new byte[]{0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // 0x04 删除指纹命令
        temp_bytes[1] = (byte)userID; //需要增加的USER   id

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
            i = 5;
            e.printStackTrace();
        }

        if(tmp_cnt < 5) {
            return 0;
        }
        else {
            Looper.prepare();   //只能在主线程中使用toast 所以要加这两句
            Toast toast= Toast.makeText(FingerActivity.this,"CONNETCT FAILED!",Toast.LENGTH_SHORT);
            toast.show();
            Looper.loop();
            return -1;
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

    @Override
    protected void onDestroy() { //Activity退出时会调用，handle内存泄露
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
    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {      //判断标志位
                case MSG_SUCCESS:
                    //成功
                    if(fingerState == FINGER_STATE_ADD)
                    {
                        //更新UI
                        userInfo[userID - 1] = '1';
                        fingerID[userID - 1].setBackgroundResource(R.drawable.tick_select);
                        //写入文件

                        Log.e("fff", "添加成功，写入文件。。。。。。");
                        String str = new String(userInfo);
                        writeTxtToFile(str, filePath, fileName);
                        //提示成功
                        fingerTextView.setText("添加用户成功！");
                        fingerTextView.setVisibility(View.VISIBLE);
                        fingerState = FINGER_STATE_DEF;  //清空状态

                    }else if(fingerState == FINGER_STATE_DEL)
                    {
                        userInfo[userID - 1] = '0';
                        fingerID[userID - 1].setBackgroundResource(R.drawable.pick_select);
                        //写入文件

                        String str = new String(userInfo);
                        writeTxtToFile(str, filePath, fileName);
                        Log.e("fff", "删除成功，写入文件。。。。。。");
                        //提示成功
                        fingerTextView.setText("删除用户成功！");
                        fingerTextView.setVisibility(View.VISIBLE);
                        fingerState = FINGER_STATE_DEF;  //清空状态
                    }
                    break;
                case MSG_FAILED: //失败
                    if(fingerState == FINGER_STATE_ADD)
                    {
                        //更新UI
                        //提示成功
                        fingerTextView.setText("添加用户失败！");
                        fingerTextView.setVisibility(View.VISIBLE);
                        fingerState = FINGER_STATE_DEF;  //清空状态

                    }else if(fingerState == FINGER_STATE_DEL)
                    {
                        fingerTextView.setText("删除用户失败！");
                        fingerTextView.setVisibility(View.VISIBLE);
                        fingerState = FINGER_STATE_DEF;  //清空状态
                    }
                    break;

                case MSG_AGAIN: //再试一次
                    if(fingerState == FINGER_STATE_ADD) {
                        fingerTextView.setText("请松手再按一次！");
                        fingerTextView.setVisibility(View.VISIBLE);
                    }
                    break;
                case MSG_INVISIABLE: //隐藏TextView
                    fingerTextView.setVisibility(View.INVISIBLE);
                    break;
                default:
                    break;
            }
        }
    };

    FingerActivity.WorkThread myThread = new FingerActivity.WorkThread();  //串口接收指纹数据
    volatile  int invisable_cnt = 0;
    public class WorkThread extends Thread {

        @Override
        public void run() {
            super.run();

            /**
             耗时操作
             */

            while(!isInterrupted()) {
                if(checkRecPack() == 0) {
                    Message msg = Message.obtain();
                    if (ackCheck == ACK_SUCCESS) //判断应答是否正确
                    {
                        handler.sendEmptyMessageDelayed(MSG_SUCCESS, 1);  //发送消息  成功信号
                    } else if (ackCheck == ACK_FAILED) //判断应答错误
                    {
                        handler.sendEmptyMessageDelayed(MSG_FAILED, 1);  //发送指纹识别错误消息
                    }else if (ackCheck == ACK_AGAIN) //再次ACK_AGAIN,       //再次输入
                    {
                        handler.sendEmptyMessageDelayed(MSG_AGAIN, 1);  //
                    } else {  //异常

                    }
                        TimerTask task = new TimerTask() {
                            @Override
                            public void run() {
                                /**
                                 *要执行的操作
                                 */
                                if(invisable_cnt > 0) {

                                    Log.e("fff", "x 大于0次 。。。。。。");
                                    Toast toast= Toast.makeText(FingerActivity.this,"大于0次！!",Toast.LENGTH_SHORT);
                                    toast.show();
                                    invisable_cnt--;
                                }else if(invisable_cnt == 0) {  //两个隐藏任务会重叠，显示与不显示时间出错，。如果当前有任务还在等待隐藏这个textview，不操作，取最后一次的操作隐藏
                                    handler.sendEmptyMessageDelayed(MSG_INVISIABLE, 1);  //发送消息  成功信号
                                }
                                this.cancel();
                            }
                        };
                        Timer timer = new Timer();
                        timer.schedule(task, 4000);//300ms后执行TimeTask的run方法
                }
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        int i = 0;
        hideBottomKey();
        setContentView(R.layout.activity_finger);


        makeFilePath(filePath, fileName);
        if(readFile(filePath, fileName) != 0)
        {
            //用户文件损坏，删除再重新写入

            int result = new ShellCommandExecutor()
                    .addCommand("mount -o remount,rw /system")
                    .addCommand("rm /mnt/sdcard/SanFeng/data/userInfo")
                    .execute();

            makeFilePath(filePath, fileName);
            String str = new String(userInfo);
            writeTxtToFile(str, filePath, fileName);
        }


        changPasswdReturn  =  findViewById(R.id.finger_chang_passwd);
        fingerReturn        =  findViewById(R.id.finger_return);
        fingerDelUser       =  findViewById(R.id.finger_del_user);
        fingerAddUser       =  findViewById(R.id.finger_add_user);
        fingerID[0] 		=  findViewById(R.id.finger_user_1);
        fingerID[1] 		=  findViewById(R.id.finger_user_2);
        fingerID[2] 		=  findViewById(R.id.finger_user_3);
        fingerID[3] 		=  findViewById(R.id.finger_user_4);
        fingerID[4] 		=  findViewById(R.id.finger_user_5);
        fingerID[5] 		=  findViewById(R.id.finger_user_6);
        fingerTextView  	=  findViewById(R.id.finger_note_txt);
        // 3.设置按钮点击事件
        for(i = 0; i < 6; i++) {
            fingerID[i].setOnClickListener(onClickListener);
        }

        fingerReturn.setOnClickListener(onClickListener);
        fingerDelUser.setOnClickListener(onClickListener);
        fingerAddUser.setOnClickListener(onClickListener);
        changPasswdReturn.setOnClickListener(onClickListener);

        for(i = 0; i < 6; i++) {
            if (userInfo[i] == '1') {
                fingerID[i].setBackgroundResource(R.drawable.tick_shade);
            } else {
                fingerID[i].setBackgroundResource(R.drawable.pick_shade);
            }
        }

        init_serial();          //初始化串口
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
        timer.schedule(task, 200);//300ms后执行TimeTask的run方法  task现在只负责接收

    }

    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.finger_user_1:
                    userID = 1;
                    break;
                case R.id.finger_user_2:
                    userID = 2;
                    break;
                case R.id.finger_user_3:
                    userID = 3;
                    break;
                case R.id.finger_user_4:
                    userID = 4;
                    break;
                case R.id.finger_user_5:
                    userID = 5;
                    break;
                case R.id.finger_user_6:
                    userID = 6;
                    break;
                case R.id.finger_add_user:
                    if(userID == 0)//没有选择用户
                    {
                        fingerTextView.setText("请先选择需要增加的用户！");
                        fingerTextView.setVisibility(View.VISIBLE);

                        TimerTask task = new TimerTask() {
                            @Override
                            public void run() {
                                Message msg = Message.obtain();
                                handler.sendEmptyMessageDelayed(MSG_INVISIABLE, 1);  //发送消息  成功信号
                                this.cancel();
                            }
                        };
                        Timer timer = new Timer();
                        timer.schedule(task, 1400);//更新UI  不可见
                    }else{
                        fingerTextView.setText("请将手指按下！");
                        fingerTextView.setVisibility(View.VISIBLE);
                        fingerState = 1;     //1 状态是增加用户
                        sendAddUserPack();        /*串口发送*/
                    }
                    break;
                case R.id.finger_del_user:
                    if(userID == 0)//没有选择用户
                    {
                        fingerTextView.setText("请先选择需要删除的用户！");
                        fingerTextView.setVisibility(View.VISIBLE);
                        TimerTask task = new TimerTask() {
                            @Override
                            public void run() {
                                Message msg = Message.obtain();
                                handler.sendEmptyMessageDelayed(MSG_INVISIABLE, 1);  //发送消息  成功信号
                                this.cancel();
                            }
                        };
                        Timer timer = new Timer();
                        timer.schedule(task, 1400);//更新UI  不可见
                    }else
                    {
                        fingerState = 2;     //1 状态是删除用户
                        sendDelUserPack();        /*串口*/
                    }
                    break;

                case R.id.finger_chang_passwd:
                    Intent intent = new Intent();
                    intent.setClass(FingerActivity.this, ChangPasswdActivity.class);
                    FingerActivity.this.startActivity(intent);
                    break;
                case R.id.finger_return:
                    //清理
                    finish();
                    break;
                default:
                    break;
            }

            if(userIDSave != userID){
                fingerTextView.setVisibility(View.INVISIBLE);
                if(userIDSave  != 0){ //第一次进来
                    if (userInfo[userIDSave - 1] == '1') {
                        fingerID[userIDSave - 1].setBackgroundResource(R.drawable.tick_shade);
                    } else {
                        fingerID[userIDSave - 1].setBackgroundResource(R.drawable.pick_shade);
                    }
                }

                if (userInfo[userID - 1] == '1') {
                    fingerID[userID - 1].setBackgroundResource(R.drawable.tick_select);
                } else {
                    fingerID[userID - 1].setBackgroundResource(R.drawable.pick_select);
                }

                userIDSave = userID;
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
            Log.e("TestFile", "Error on write File:" + e);
        }
    }

    /**
     * 读入TXT文件
     */
    public int readFile(String filePath, String fileName) {
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
                len = line.length();   //指纹信息，0 未录入 1 已经录入
                if(len != 6)
                {
                    userInfo = "000000".toCharArray();
                    Toast toast= Toast.makeText(FingerActivity.this,"用户指纹文件损坏！!",Toast.LENGTH_SHORT);
                    toast.show();
                    return -1;
                }

                userInfo = line.toCharArray();
//
//                //简单加密,先小写后大写，放到ASCII后面看不到的数据就可以
//                for(i = 0; i < 6; i++) {
//                    charArry[i] = (char)((int)charArry[i] + (int)'!');
//                    //charArry[i] = (char)((int)charArry[i] + (int)'!');
//                }
//                g_passwd = String.valueOf(charArry);

            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return 0;
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

        public FingerActivity.ShellCommandExecutor addCommand(String cmd) {
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
