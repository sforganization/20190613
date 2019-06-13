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

public class ModeSetActivity extends AppCompatActivity {

    protected  int glass_num  = 0;
    protected  boolean inputCheck = true;   //输入是否有效值
    private byte[] glassData = new byte[50];    //一个完整数据包

    protected  final  byte MOTO_TIME_TPD    = 0;
    protected  final  byte MOTO_TIME_650    = 1;
    protected  final  byte MOTO_TIME_750    = 2;
    protected  final  byte MOTO_TIME_850    = 3;
    protected  final  byte MOTO_TIME_1000   = 4;
    protected  final  byte MOTO_TIME_1950   = 5;

    protected  final  byte MOTO_FR_FWD      = 0;
    protected  final  byte MOTO_FR_REV      = 1;
    protected  final  byte MOTO_FR_FWD_REV  = 2;

    private  byte tpd_mode = 0; //tap 模式
    private  byte dir_mode = 0; //转动方向

    Button tpd_650 = null;
    Button tpd_750 = null;
    Button tpd_850 = null;
    Button tpd_1000 =null;
    Button tpd_1950 =null;
    Button right_button =null;
    Button left_button = null;
    Button right_left = null;

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
            serialttyS0 = new com.example.x6.serial.SerialPort(new File("/dev/ttyS2"),115200,0, 20);
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

    private byte[] recvData = new byte[128];    //每次最大是32位
    private byte[] recvArry = new byte[256];    //设置缓存大概4级    256/ 53 ~ 4
    private int i= 0;
    private int index= 0;
    private int cnt= 0;
    private int sizeRec= 0;
    private byte checkSum = 0;

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

    protected int checkRecPack() {   //串口接收数据

        int tmp_cnt = 0;

        init_serial();          //初始化串口
        sendReadyPack();        /*串口发送就绪包*/
        try {
            for(tmp_cnt = 0; tmp_cnt < 5; tmp_cnt++) //连续发5次，不行就是认为单片机已经坏，或者接线已经松开
            {
                sizeRec = 0;
                do {
                    cnt = -1;
                    if(ttyS0InputStream != null)
                        cnt = ttyS0InputStream.read(recvData);
                    System.arraycopy(recvData, 0, recvArry, sizeRec, cnt);
                    sizeRec += cnt;
                } while (sizeRec < 53);  //少于一个包数据

                if(jungleRecPack() == 0) //检查参数合法性
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

        if(tmp_cnt < 5) {
            return 0;
        }
        else {
            Looper.prepare();   //只能在主线程中使用toast 所以要加这两句
            Toast toast= Toast.makeText(ModeSetActivity.this,"CONNETCT FAILED!",Toast.LENGTH_SHORT);
            toast.show();
            Looper.loop();
            return -1;
        }
    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {      //判断标志位
                case 1:
                    /**
                     获取数据，更新UI
                     */
                    Intent intent = new Intent();
                    Bundle myBudle=new Bundle();
                    myBudle.putByteArray("glassData", glassData);
                    intent.putExtras(myBudle);

                    if((glassData[1] & 0x7F) == 16)   //6个手表
                    {
                        Toast toast= Toast.makeText(ModeSetActivity.this,"6个!",Toast.LENGTH_SHORT);
                        toast.show();
                        intent.setClass(ModeSetActivity.this, ConctrlActivity.class);
                        ModeSetActivity.this.startActivity(intent);
                        finish();
                    }
                    else
                    {
                        Toast toast= Toast.makeText(ModeSetActivity.this,"16个!",Toast.LENGTH_SHORT);
                        toast.show();
                        intent.setClass(ModeSetActivity.this, Conctrl_16_Activity.class);
                        ModeSetActivity.this.startActivity(intent);
                        finish();
                    }
                    break;
                default:
                    break;
            }
        }
    };

    public class WorkThread extends Thread {

        @Override
        public void run() {
            super.run();
            /**
             耗时操作
             */
            checkRecPack();
            //从全局池中返回一个message实例，避免多次创建message（如new Message）
            Message msg =Message.obtain();
            msg.obj = glassData;
            msg.what=1;   //标志消息的标志
            handler.sendMessage(msg);  //发送消息
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
    public void readFile(String filePath, String fileName) {
        int i, len;
        char charArry[] = new char[60];
        byte[] tem = new byte[50];    //一个完整数据包
        boolean checkBool =  false;

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
                if(len != 50)
                {
                    Toast toast= Toast.makeText(ModeSetActivity.this,"文件损坏！!",Toast.LENGTH_SHORT);
                    toast.show();
                    break;
                }
                //由于char是16位宽与string一致，所以如果用byte是1开头的会多占一个字节长度
                charArry = line.toCharArray();

                for(i = 0; i < 50; i++){
                    glassData[i] = (byte)charArry[i];
                }

                for(i = 0; i < 16; i++){
                    if((glassData[2 + (i * 3) + 1] < MOTO_FR_FWD) || (glassData[2 + (i * 3) + 1] > MOTO_FR_FWD_REV)) {
                        checkBool = true;
                        glassData[2 + (i * 3) + 1] = MOTO_FR_FWD;
                    }

                    if ((glassData[2 + (i * 3) + 2] < MOTO_TIME_TPD) || (glassData[2 + (i * 3) + 2] > MOTO_TIME_1950)) {
                        checkBool = true;
                        glassData[2 + (i * 3) + 2] = MOTO_TIME_650;
                    }
                }

                if(checkBool == true){  //数据有异常，改写后重新写入
                    byte[] glassDataWrite = new byte[50];    //一个完整数据包

                    //数据正确，写入文件
                    String filePath_data = "/mnt/sdcard/SanFeng/data/";
                    String fileName_data = "stateInfo";   //保存手表状态信息

                    glassDataWrite = glassData.clone();
                    glassDataWrite[0] = (byte) (glassDataWrite[0] ^ (byte) 0x80);  //去除最高位
                    String str = new String(glassDataWrite);
                    writeTxtToFile(str, filePath_data, fileName_data);
                }

                Log.d("fff", "main haha");

                return;   //直接返回，只读第一行
            }
        } catch (IOException e) {
            e.printStackTrace();
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

        public ModeSetActivity.ShellCommandExecutor addCommand(String cmd) {
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

    public void changeViewShade() {

        tpd_650.setBackgroundResource(R.drawable.mode_set_button_shade);
        tpd_750.setBackgroundResource(R.drawable.mode_set_button_shade);
        tpd_850.setBackgroundResource(R.drawable.mode_set_button_shade);
        tpd_1000.setBackgroundResource(R.drawable.mode_set_button_shade);
        tpd_1950.setBackgroundResource(R.drawable.mode_set_button_shade);
        right_button.setBackgroundResource(R.drawable.mode_set_button_shade);
        left_button.setBackgroundResource(R.drawable.mode_set_button_shade);
        right_left.setBackgroundResource(R.drawable.mode_set_button_shade);

        switch (dir_mode)
        {
            case MOTO_FR_FWD:
                right_button.setBackgroundResource(R.drawable.mode_set_button);
                break;
            case MOTO_FR_REV:
                left_button.setBackgroundResource(R.drawable.mode_set_button);
                break;
            case MOTO_FR_FWD_REV:
                right_left.setBackgroundResource(R.drawable.mode_set_button);
                break;
            default:
                break;
        }

        switch (tpd_mode)
        {
            case MOTO_TIME_650:
                tpd_650.setBackgroundResource(R.drawable.mode_set_button);
                break;
            case MOTO_TIME_750:
                tpd_750.setBackgroundResource(R.drawable.mode_set_button);
                break;
            case MOTO_TIME_850:
                tpd_850.setBackgroundResource(R.drawable.mode_set_button);
                break;
            case MOTO_TIME_1000:
                tpd_1000.setBackgroundResource(R.drawable.mode_set_button);
                break;
            case MOTO_TIME_1950:
                tpd_1950.setBackgroundResource(R.drawable.mode_set_button);
                break;
            default:
                break;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        //关闭串口
        try {
            ttyS0InputStream.close();
            Log.d("aaa", "modeset 关out。。。。。。");
        } catch (IOException e) {
            e.printStackTrace();
        }

        try {
            ttyS0OutputStream.close();
            Log.d("aaa", "modeset 关in。。。。。。");
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (serialttyS0 != null) {
            serialttyS0.close();
            Log.d("fff", "modeset 串口不为空 关闭串口。。。。。。");
            serialttyS0 = null;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        hideBottomKey();
        Bundle bundle = this.getIntent().getExtras();
        glass_num = bundle.getInt("glass_num");
        setContentView(R.layout.activity_mode_set);

        String filePath = "/mnt/sdcard/SanFeng/data/";
        String fileName = "stateInfo";
        makeFilePath(filePath, fileName);

        readFile(filePath, fileName);
        //Button tpd_650 = (Button) findViewById(R.id.);

        dir_mode = glassData[2 + 3 * (glass_num) + 1];
        tpd_mode = glassData[2 + 3 * (glass_num) + 2];

       tpd_650      = findViewById(R.id.modeSet_650);
       tpd_750      = findViewById(R.id.modeSet_750);
       tpd_850      = findViewById(R.id.modeSet_850);
       tpd_1000     = findViewById(R.id.modeSet_1000);
       tpd_1950     = findViewById(R.id.modeSet_1950);
       right_button = findViewById(R.id.modeSet_R);
       left_button  = findViewById(R.id.modeSet_L);
       right_left   = findViewById(R.id.modeSet_RL);
       Button return_button = findViewById(R.id.modeSet_ruturn);


        // 3.设置按钮点击事件
        tpd_650.setOnClickListener(onClickListener);
        tpd_750.setOnClickListener(onClickListener);
        tpd_850.setOnClickListener(onClickListener);
        tpd_1000.setOnClickListener(onClickListener);
        tpd_1950.setOnClickListener(onClickListener);
        right_button.setOnClickListener(onClickListener);
        left_button.setOnClickListener(onClickListener);
        right_left.setOnClickListener(onClickListener);
        return_button.setOnClickListener(onClickListener);

        changeViewShade();
        init_serial();          //初始化串口
    }


    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {

            byte[] temp_bytes = new byte[]{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // 0x02 更新状状态命令

            inputCheck = true; //默认有效
            switch (v.getId()) {
                case R.id.modeSet_650:
                    tpd_mode = MOTO_TIME_650;
                    break;
                case R.id.modeSet_750:
                    tpd_mode = MOTO_TIME_750;
                    break;
                case R.id.modeSet_850:
                    tpd_mode = MOTO_TIME_850;
                    break;
                case R.id.modeSet_1000:
                    tpd_mode = MOTO_TIME_1000;
                    break;
                case R.id.modeSet_1950:
                    tpd_mode = MOTO_TIME_1950;
                    break;
                case R.id.modeSet_R:
                    dir_mode = MOTO_FR_FWD;
                    break;
                case R.id.modeSet_L:
                    dir_mode = MOTO_FR_REV;
                    break;
                case R.id.modeSet_RL:  //正反转
                    dir_mode = MOTO_FR_FWD_REV;
                    break;
                case R.id.modeSet_ruturn:
                    inputCheck = false; //无效输入

                    //数据正确，写入文件
                    String filePath_data = "/mnt/sdcard/SanFeng/data/";
                    String fileName_data = "stateInfo";   //保存手表状态信息

                    String str = new String(glassData);
                    writeTxtToFile(str, filePath_data, fileName_data);

                    Toast toast= Toast.makeText(ModeSetActivity.this,"写入文件！",Toast.LENGTH_SHORT);
                    toast.show();
                    TimerTask task = new TimerTask() {
                        @Override
                        public void run() {
                            /**
                             *要执行的操作
                             */
                            //清理
                            finish();
                            this.cancel();
                        }
                    };

                    Timer timer = new Timer();
                    timer.schedule(task, 200);//300ms后执行TimeTask的run方法   //要延时一定的时间去操作这个复制文件，不然复制不成功
                    break;
                default:
                    inputCheck = false; //无效输入
                    break;
            }

            //退出时再保存到文件中

            if(inputCheck == true){
            glassData[2 + 3 * (glass_num) + 1] = dir_mode;
            glassData[2 + 3 * (glass_num) + 2] = tpd_mode;

            changeViewShade();

            temp_bytes[0] = (byte)0x02;       //0x02 更新状状态命令
            temp_bytes[1] = (byte)glass_num; //地址 参数0  地址，LED模式，增加删除指纹ID,
            temp_bytes[5] = dir_mode;  //方向
            temp_bytes[6] = tpd_mode;  //时间

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

    private void sendUpDataPack() {  //发送包

    }
}
