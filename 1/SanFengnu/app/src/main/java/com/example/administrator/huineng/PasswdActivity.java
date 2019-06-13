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
import android.widget.ImageView;
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

public class PasswdActivity extends AppCompatActivity {


    public static   String g_passwd = new String("");
    protected  int glass_num = 0;
    protected  int inputCnt = 0;   //输入记数
    protected  String inputNum = new String();   //输入值
    protected  String inputtmp = new String();   //输入值
    protected  boolean inputCheck = true;   //输入是否有效值

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

    @Override
    protected void onDestroy() { //Activity退出时会调用，handle内存泄露
        super.onDestroy();
    }
//
//    @SuppressLint("HandlerLeak")
//    private Handler handler = new Handler(){
//        @Override
//        public void handleMessage(Message msg) {
//            super.handleMessage(msg);
//            switch (msg.what) {      //判断标志位
//                case 5:
//                    /**
//                     获取数据，更新UI
//                     */
//                    //密码正确，写入
//                    Intent intent = new Intent();
//                    Bundle myBudle=new Bundle();
//                    myBudle.putInt("glass_num", glass_num);
//                    intent.putExtras(myBudle);
//
//                    // 移除所有消息
//                    handler.removeCallbacksAndMessages(null);
//                    // 或者移除单条消息
//                    handler.removeMessages(msg.what);
//
//                    Toast toast= Toast.makeText(PasswdActivity.this,"2333 try wearing!",Toast.LENGTH_SHORT);
//                    toast.show();
//
//                    intent.setClass(PasswdActivity.this, TryingWearActivity.class);
//                    startActivity(intent);
//                    finish();
//                    break;
//                case 6: //未识别到的指纹
//                    /**
//                     获取数据，更新UI
//                     */
//                    input_6.setVisibility(View.VISIBLE);
//                    input_5.setVisibility(View.VISIBLE);
//                    input_4.setVisibility(View.VISIBLE);
//                    input_3.setVisibility(View.VISIBLE);
//                    input_2.setVisibility(View.VISIBLE);
//                    input_1.setVisibility(View.VISIBLE);
//
//                    error_info.setVisibility(View.VISIBLE);
//                    break;
//                default:
//                    break;
//            }
//            if(inputNum.length() > 0) {
//                inputNum = "";
//                inputCnt = 0;
//            }
//        }
//    };
//
//    PasswdActivity.WorkThread myThread = new PasswdActivity.WorkThread();  //串口接收指纹数据
//
//
//    public class WorkThread extends Thread {
//
//        @Override
//        public void run() {
//            super.run();
//            /**
//             耗时操作
//             */
//            while(!isInterrupted()) {
//                if(checkRecPack() == 0) {
//                    //从全局池中返回一个message实例，避免多次创建message（如new Message）
//                    Message msg = Message.obtain();
////            msg.obj = glassData;
////            msg.what=1;   //标志消息的标志
//
//
//                    if (ackCheck == 0) //判断应答是否正确
//                    {
//                        handler.sendEmptyMessageDelayed(5, 1);  //发送消息
//                    } else if (ackCheck == 1) //判断应答错误
//                    {
//                        handler.sendEmptyMessageDelayed(6, 1);  //发送指纹识别错误消息
//                    } else {  //异常
//
//                    }
//                }
//            }
//        }
//    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){

        int msgNum = 0;
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            msgNum = msg.what;

            if(msgNum == 0x8800){ //不显示
                error_info.setVisibility(View.INVISIBLE);

                if(showTimece == 0 ) {
                    for (; inputNum.length() > 0; ) {
                        inputNum = inputNum.substring(0, inputNum.length() - 1);
                        inputCnt--;
                    }

                    input_1.setVisibility(View.INVISIBLE);
                    input_2.setVisibility(View.INVISIBLE);
                    input_3.setVisibility(View.INVISIBLE);
                    input_4.setVisibility(View.INVISIBLE);
                    input_5.setVisibility(View.INVISIBLE);
                    input_6.setVisibility(View.INVISIBLE);
                }
            }else if(msgNum == 0x9900){//显示
                error_info.setVisibility(View.VISIBLE);
            }
        }
    };

    protected  int showTimece = 0; //闪烁次数
    private   volatile  int tickTime = 0;
    Timer timer = null;
    TimerTask tick_task= null;

    private void startTimer(){
        if (timer == null) {
            timer = new Timer();
        }

        if (tick_task == null) {
            tick_task = new TimerTask() {
                @Override
                public void run() {
                    /**
                     *要执行的操作
                     */
                    tickTime++;
                    if(tickTime >= 150)//闪烁频率
                    {
                        tickTime = 0;
                        if (showTimece != 0) {
                            showTimece--;
                            if((showTimece & 0x01) == 0){
                                Message msg = Message.obtain();
                                handler.sendEmptyMessageDelayed((0x8800), 1);  //发送消息 不显示
                            } else{

                                Message msg = Message.obtain();
                                handler.sendEmptyMessageDelayed((0x9900), 1);  //发送消息
                            }
                    }
                    }
                }
            };
        }

        if(timer != null)
        {
            if(tick_task != null ) {
                timer.schedule(tick_task, 10, 1);//1ms后执行Tick   1ms 的tick
            }

        }
    }

    private void stopTimer(){

        if (tick_task != null) {
            tick_task.cancel();
            tick_task = null;
        }

        if (timer != null) {
            timer.cancel();
            timer = null;
        }
        System.gc();
    }


    private  boolean bChangePasswd = false;
    TextView input_1 = null;
    TextView input_2 = null;
    TextView input_3 = null;
    TextView input_4 = null;
    TextView input_5 = null;
    TextView input_6 = null;
    ImageView error_info = null;

    Button input_00_Button = null;
    Button input_01_Button = null;
    Button input_02_Button = null;
    Button input_03_Button = null;
    Button input_04_Button = null;
    Button input_05_Button = null;
    Button input_06_Button = null;
    Button input_07_Button = null;
    Button input_08_Button = null;
    Button input_09_Button = null;
    Button passwd_del_button = null;
    Button passwd_return_button = null;
    Button change_passwd = null;
    Button original_passwd = null;
    ImageView employ_image = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomKey();
        setContentView(R.layout.activity_passwd);

        glass_num = 1;

        String filePath = "/mnt/sdcard/SanFeng/data/";
        String fileName = "loginfo";
        makeFilePath(filePath, fileName);

        //writeTxtToFile("666666", filePath, fileName);
        readFile(filePath, fileName);

       input_00_Button = findViewById(R.id.passwd_digit_00);
       input_01_Button = findViewById(R.id.passwd_digit_01);
       input_02_Button = findViewById(R.id.passwd_digit_02);
       input_03_Button = findViewById(R.id.passwd_digit_03);
       input_04_Button = findViewById(R.id.passwd_digit_04);
       input_05_Button = findViewById(R.id.passwd_digit_05);
       input_06_Button = findViewById(R.id.passwd_digit_06);
       input_07_Button = findViewById(R.id.passwd_digit_07);
       input_08_Button = findViewById(R.id.passwd_digit_08);
       input_09_Button = findViewById(R.id.passwd_digit_09);
       passwd_del_button = findViewById(R.id.passwd_del_button);
        passwd_return_button = findViewById(R.id.passwd_ruturn);
        change_passwd = findViewById(R.id.passwd_change);
        original_passwd = findViewById(R.id.passwd_original);
        employ_image  = findViewById(R.id.passwd_employ);
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
        passwd_del_button.setOnClickListener(onClickListener);
        passwd_return_button.setOnClickListener(onClickListener);
        change_passwd.setOnClickListener(onClickListener);

        input_1 = findViewById(R.id.passwd_input_1);
        input_2 = findViewById(R.id.passwd_input_2);
        input_3 = findViewById(R.id.passwd_input_3);
        input_4 = findViewById(R.id.passwd_input_4);
        input_5 = findViewById(R.id.passwd_input_5);
        input_6 = findViewById(R.id.passwd_input_6);
        error_info  = findViewById(R.id.passwd_error_frame);

//        TimerTask task = new TimerTask() {
//            @Override
//            public void run() {
//                /**
//                 *要执行的操作
//                 */
//                myThread.start();        /*延时一段时间再开启线程*/
//                this.cancel();
//            }
//        };
//        Timer timer = new Timer();
//        timer.schedule(task, 200);//300ms后执行TimeTask的run方法
        startTimer();
    }

    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            inputCheck = true; //默认有效
            switch (v.getId()) {
                case R.id.passwd_digit_00:
                    if(inputCnt < 6)
                        inputNum += "0";
                    break;
                case R.id.passwd_digit_01:
                    if(inputCnt < 6)
                        inputNum += "1";
                    break;
                case R.id.passwd_digit_02:
                    if(inputCnt < 6)
                        inputNum += "2";
                    break;
                case R.id.passwd_digit_03:
                    if(inputCnt < 6)
                        inputNum += "3";
                    break;
                case R.id.passwd_digit_04:
                    if(inputCnt < 6)
                        inputNum += "4";
                    break;
                case R.id.passwd_digit_05:
                    if(inputCnt < 6)
                        inputNum += "5";
                    break;
                case R.id.passwd_digit_06:
                    if(inputCnt < 6)
                        inputNum += "6";
                    break;
                case R.id.passwd_digit_07:
                    if(inputCnt < 6)
                        inputNum += "7";
                    break;
                case R.id.passwd_digit_08:
                    if(inputCnt < 6)
                        inputNum += "8";
                    break;
                case R.id.passwd_digit_09:
                    if(inputCnt < 6)
                        inputNum += "9";
                    break;
                case R.id.passwd_del_button:
                    inputCheck = false; //无效输入
                    if(inputNum.length() > 0) {
                        inputNum = inputNum.substring(0, inputNum.length() - 1);
                        inputCnt--;
                    }
                    break;
                case R.id.passwd_change:
                    inputCheck = false; //无效输入
                    for(; inputNum.length() > 0 ; inputNum.length()) {
                        inputNum = inputNum.substring(0, inputNum.length() - 1);
                        inputCnt--;
                    }
                    //点击这个修改密码 先输入原始密码， 这时候先了隐藏修改密码按键
                    bChangePasswd = true;
                    change_passwd.setVisibility(View.INVISIBLE);
                    employ_image.setVisibility(View.INVISIBLE);
                    original_passwd.setVisibility(View.VISIBLE);
                    break;
                case R.id.passwd_ruturn:
                    inputCheck = false; //无效输入
                    //清理
                    inputCnt = 0;
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
                    //error_info.setVisibility(View.VISIBLE);
                    showTimece = 4;
                }
                else{ //密码正确

                    Intent intent = new Intent();
                    if(bChangePasswd){   //如果前面 点了修改密码
                        intent.setClass(PasswdActivity.this, ChangPasswdActivity.class);
                    }else {
                        Bundle myBudle = new Bundle();
                        myBudle.putInt("glass_num", glass_num);
                        intent.putExtras(myBudle);
                        intent.setClass(PasswdActivity.this, TryingWearActivity.class);
                    }
                    PasswdActivity.this.startActivity(intent);
                    finish();
                }
            }
            else{
                error_info.setVisibility(View.INVISIBLE);
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
            Log.d("TestFile", "Error on write File:" + e);
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
                    Toast toast= Toast.makeText(PasswdActivity.this,"密码文件损坏！!",Toast.LENGTH_SHORT);
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

        public PasswdActivity.ShellCommandExecutor addCommand(String cmd) {
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
