package com.example.administrator.huineng;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
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

import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;

public class SelectActivity extends AppCompatActivity {

    protected  int glass_num = 0;
    private byte[] glassData = new byte[50];    //一个完整数据包


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

    volatile int movieTimes = 0;

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(requestCode == 1){
            movieTimes = 0;
            isReady = true;
            startTimer();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        int i;


        tick_task.cancel();
        serial_task.cancel();
        // 移除所有消息
        handler.removeCallbacksAndMessages(null);
        // 或者移除单条消息
        handler.removeMessages(0x1100);
    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){

        int msgNum = 0;
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            msgNum = msg.what;


            stopTimer();
            handler.removeCallbacksAndMessages(null);
            Intent intent = new Intent();

            Bundle bundle = new Bundle();
            bundle.putInt("glass_num", glass_num);

//            Bundle bundle = new Bundle();
//            Intent intent = new Intent(Function_16_Activity.this, ModeSetActivity.class);
//            bundle.putInt("glass_num", glass_num);
//            intent.putExtras(bundle);
//            startActivity(intent);

            if(msgNum == 0x1100){ //广告
                //g_exit = true;   //只暂停， 停止线程会出现 问题
                intent.setClass(SelectActivity.this, Advertisment.class);
            }else if(msgNum == 0xaa00){
                intent.setClass(SelectActivity.this, HongKong.class);
            }else if(msgNum == 0xdd00) {  //USB 复制文件
                copyImageFile();
                return;
            }

            intent.putExtras(bundle);
            startActivityForResult(intent, 1);
        }
    };

    Timer timer = null;
    TimerTask tick_task= null;
    TimerTask serial_task= null;
    volatile boolean isReady = true;

    private void startTimer(){
        if (timer == null) {
            timer = new Timer();
        }

        if (tick_task == null) {
            tick_task = new TimerTask() {
                @Override
                public void run() {

                    if(isReady == false)
                        return;

                    /**
                     *要执行的操作
                     */
                    movieTimes++;
                    if(movieTimes >= 10000)//屏幕没有操作跳到广告页面
                    {
                        movieTimes = 0;
                        isReady = false;
                        Message msg = Message.obtain();
                        handler.sendEmptyMessageDelayed((0x1100), 1);  //发送消息
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

        isReady = false;

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

    private Button watch_a = null;
    private Button watch_b = null;
    private Button watch_c = null;
    private Button watch_d = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideBottomKey();

        Bundle bundle = this.getIntent().getExtras();
        glassData = bundle.getByteArray("glassData");

        String filePath = "/mnt/sdcard/SanFeng/data/";
        //新建一个File，传入文件夹目录
        makeRootDirectory(filePath);

        setContentView(R.layout.activity_select);


        initViews();

        startTimer();
        //detectUsbWithBroadcast();  //检测USB接入

    }

    private void initViews() {
        //这是获得include中的控件
        watch_a  = findViewById(R.id.select_a);
        watch_b  = findViewById(R.id.select_b);
        watch_c  = findViewById(R.id.select_c);
        watch_d  = findViewById(R.id.select_d);
        // 3.设置按钮点击事件
        watch_a.setOnClickListener(onClickListener);
        watch_b.setOnClickListener(onClickListener);
        watch_c.setOnClickListener(onClickListener);
        watch_d.setOnClickListener(onClickListener);
    }

    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {

        boolean inputCheck2 = false;

        @Override
        public void onClick(View v) {

            movieTimes = 0;  //清空这个视频时间，有按下重新开始计时
            isReady = false;
            Message msg = Message.obtain();

            switch (v.getId()) {
                case R.id.select_a:
                    glass_num = 0;
                    handler.sendEmptyMessageDelayed((0xaa00), 1);  //发送消息
                    break;

                case R.id.select_b:
                    glass_num = 1;
                    handler.sendEmptyMessageDelayed((0xaa00), 1);  //发送消息
                    break;

                case R.id.select_c:
                    glass_num = 2;
                    handler.sendEmptyMessageDelayed((0xaa00), 1);  //发送消息
                    break;

                case R.id.select_d:
                    glass_num = 3;
                    handler.sendEmptyMessageDelayed((0xaa00), 1);  //发送消息
                    break;


                default:
                    break;
            }
        }
    };

    public int copyImageFile()
    {
        /**要执行的操作*/
        ShellCommandExecutor mShellCmd = new ShellCommandExecutor();

        mShellCmd.addCommand("mount -o remount,rw /system");

        if(fileIsExists("/mnt/usbhost1/movies/video1.mp4"))
        {
            Log.e("error:", "movies 存在 。。。");
            mShellCmd.addCommand("cat /mnt/usbhost1/movies/video1.mp4 > /mnt/sdcard/SanFeng/movies/lanbo.mp4");
        }else{
            Log.e("error:", "movies 不存在！！");
        }

        if(fileIsExists("/mnt/usbhost1/image/h1.png"))
        {
            Log.e("error:", "文件存在  H1 。。。");
            mShellCmd.addCommand("cat /mnt/usbhost1/image/h1.png > /mnt/sdcard/SanFeng/image/h1.png");
        }else{
            Log.e("error:", "文件 H1 不存在！！");
        }

        if(fileIsExists("/mnt/usbhost1/image/h2.png"))
        {
            Log.e("error:", "文件存在  H2 。。。");
            mShellCmd.addCommand("cat /mnt/usbhost1/image/h2.png > /mnt/sdcard/SanFeng/image/h2.png");
        }else{
            Log.e("error:", "文件 H2不存在！！");
        }

        if(fileIsExists("/mnt/usbhost1/image/h3.png"))
        {
            Log.e("error:", "文件存在  H3 。。。");
            mShellCmd.addCommand("cat /mnt/usbhost1/image/h3.png > /mnt/sdcard/SanFeng/image/h3.png");
        }else{
            Log.e("error:", "文件 H3 不存在！！");
        }

        if(fileIsExists("/mnt/usbhost1/image/h4.png"))
        {
            Log.e("error:", "文件存在  h4 。。。");
            mShellCmd.addCommand("cat /mnt/usbhost1/image/h4.png > /mnt/sdcard/SanFeng/image/h4.png");
        }else{
            Log.e("error:", "文件 h4不存在！！");
        }
        int result = mShellCmd.execute();
        return result;
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

    //判断文件是否存在
    public boolean fileIsExists(String strFile) {
        try {
            File f = new File(strFile);
            if (!f.exists()) {
                return false;
            }

        } catch (Exception e) {
            return false;
        }

        return true;
    }

        TimerTask task = new TimerTask() {
        @Override
        public void run() {

            //this.cancel();
        }
    };

    private BroadcastReceiver mUsbStateChangeReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d("sgg", "onReceive: " + intent.getAction());
            String action = intent.getAction();

            if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                //拔出
            }

            if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) { //插入
                UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                if (device != null) {
                    // call your method that cleans up and closes communication with the device
                    movieTimes = 0;
                    handler.sendEmptyMessageDelayed((0xdd00), 6000);  ////最小5秒后执行run方法 不然读取不到
                }
            }
        }
    };

    private void detectUsbWithBroadcast() {
        Log.d("sgg", "listenUsb: register");
        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(UsbManager.ACTION_USB_ACCESSORY_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_ACCESSORY_DETACHED);
        filter.addAction("android.hardware.usb.action.USB_STATE");

        registerReceiver(mUsbStateChangeReceiver, filter);
        Log.d("sgg", "listenUsb: registered");
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
     *                  .addCommand("cp /sdcard/Download/bootanimation.zip /system/media")  //安卓没有cp命令
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

        public ShellCommandExecutor addCommand(String cmd) {
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
