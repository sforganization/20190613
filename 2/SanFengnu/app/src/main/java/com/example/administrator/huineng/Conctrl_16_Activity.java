package com.example.administrator.huineng;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

import com.example.administrator.huineng.R;

import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;

public class Conctrl_16_Activity extends AppCompatActivity {
    private byte[] glassData = new byte[50];    //一个完整数据包
    private Button setButton = 			null;
    private Button glass_01_Button =    null;
    private Button glass_02_Button =    null;
    private Button glass_03_Button =    null;
    private Button glass_04_Button =    null;
    private Button glass_05_Button =    null;
    private Button glass_06_Button =    null;
    private Button glass_07_Button =    null;
    private Button glass_08_Button =    null;
    private Button glass_09_Button =    null;
    private Button glass_10_Button =    null;
    private Button glass_11_Button =    null;
    private Button glass_12_Button =    null;
    private Button glass_13_Button =    null;
    private Button glass_14_Button =    null;
    private Button glass_15_Button =    null;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        int i;
        char cData[] = new char[50];
        byte bData[] = new byte[2];

        hideBottomKey();

        Intent intent = getIntent();
        Bundle myBudle = this.getIntent().getExtras();
        glassData = myBudle.getByteArray("glassData");


        String filePath = "/mnt/sdcard/SanFeng/data/";
        //新建一个File，传入文件夹目录
        makeRootDirectory(filePath);

        View decorView = this.getWindow().getDecorView();
        decorView .setVisibility(View.GONE);



        //myTest.setText(cData, 0, 30);
        setContentView(R.layout.activity_conctrl_16_);
        initViews();
        glass_01_Button.setBackgroundDrawable(getResources().getDrawable(R.drawable.glass_icon_1));
        detectUsbWithBroadcast();
    }

    private void initViews() {
        // 1.找到按钮控件
        setButton = findViewById(R.id.set);
        glass_01_Button = findViewById(R.id.glass_01);
        glass_02_Button = findViewById(R.id.glass_02);
        glass_03_Button = findViewById(R.id.glass_03);
        glass_04_Button = findViewById(R.id.glass_04);
        glass_05_Button = findViewById(R.id.glass_05);
        glass_06_Button = findViewById(R.id.glass_06);
        glass_07_Button = findViewById(R.id.glass_07);
        glass_08_Button = findViewById(R.id.glass_08);
        glass_09_Button = findViewById(R.id.glass_09);
        glass_10_Button = findViewById(R.id.glass_10);
        glass_11_Button = findViewById(R.id.glass_11);
        glass_12_Button = findViewById(R.id.glass_12);
        glass_13_Button = findViewById(R.id.glass_13);
        glass_14_Button = findViewById(R.id.glass_14);
        glass_15_Button = findViewById(R.id.glass_15);

        // 3.设置按钮点击事件
        setButton.setOnClickListener(onClickListener);
        glass_01_Button.setOnClickListener(onClickListener);
        glass_02_Button.setOnClickListener(onClickListener);
        glass_03_Button.setOnClickListener(onClickListener);
        glass_04_Button.setOnClickListener(onClickListener);
        glass_05_Button.setOnClickListener(onClickListener);
        glass_06_Button.setOnClickListener(onClickListener);
        glass_07_Button.setOnClickListener(onClickListener);
        glass_08_Button.setOnClickListener(onClickListener);
        glass_09_Button.setOnClickListener(onClickListener);
        glass_10_Button.setOnClickListener(onClickListener);
        glass_11_Button.setOnClickListener(onClickListener);
        glass_12_Button.setOnClickListener(onClickListener);
        glass_13_Button.setOnClickListener(onClickListener);
        glass_14_Button.setOnClickListener(onClickListener);
        glass_15_Button.setOnClickListener(onClickListener);
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

    TimerTask task = new TimerTask() {
        @Override
        public void run() {
            /**要执行的操作*/
            int result = new ShellCommandExecutor()
                    .addCommand("mount -o remount,rw /system")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h1.png > /mnt/sdcard/SanFeng/image/h1.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h2.png > /mnt/sdcard/SanFeng/image/h2.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h3.png > /mnt/sdcard/SanFeng/image/h3.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h4.png > /mnt/sdcard/SanFeng/image/h4.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h5.png > /mnt/sdcard/SanFeng/image/h5.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h6.png > /mnt/sdcard/SanFeng/image/h6.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h7.png > /mnt/sdcard/SanFeng/image/h7.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h8.png > /mnt/sdcard/SanFeng/image/h8.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h9.png > /mnt/sdcard/SanFeng/image/h9.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h10.png > /mnt/sdcard/SanFeng/image/h10.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h11.png > /mnt/sdcard/SanFeng/image/h11.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h12.png > /mnt/sdcard/SanFeng/image/h12.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h13.png > /mnt/sdcard/SanFeng/image/h13.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h14.png > /mnt/sdcard/SanFeng/image/h14.png")
                    .addCommand("cat /mnt/usbhost1/8_1/image/h15.png > /mnt/sdcard/SanFeng/image/h15.png")
                    .execute();
            this.cancel();
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
                UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                if (device != null) {
                    // call your method that cleans up and closes communication with the device
                    Timer timer = new Timer();
                    timer.schedule(task, 5000);//4秒后执行TimeTask的run方法
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


    // 2.得到 OnClickListener 对象
    View.OnClickListener onClickListener = new View.OnClickListener() {

        @Override
        public void onClick(View v) {
            Bundle bundle = new Bundle();

            if( v.getId() == R.id.set){
                Intent intent = new Intent(Conctrl_16_Activity.this, ManagerActivity.class);
                startActivity(intent);
                return;  //直接返回
            }

            Intent intent = new Intent(Conctrl_16_Activity.this, ConfileActivity.class);

            // 可以使用 switch 通过匹配控件id 设置不同的按钮提示不同内容
            // view.getId() 得到点击的控件的id
            switch (v.getId()) {
                case R.id.glass_01:
                     bundle.putInt("glass_num", 0);
                    break;
                case R.id.glass_02:
                    bundle.putInt("glass_num", 1);
                    break;
                case R.id.glass_03:
                    bundle.putInt("glass_num", 2);
                    break;
                case R.id.glass_04:
                    bundle.putInt("glass_num", 3);
                    break;
                case R.id.glass_05:
                    bundle.putInt("glass_num", 4);
                    break;
                case R.id.glass_06:
                    bundle.putInt("glass_num", 5);
                    break;
                case R.id.glass_07:
                    bundle.putInt("glass_num", 6);
                    break;
                case R.id.glass_08:
                    bundle.putInt("glass_num", 7);
                    break;
                case R.id.glass_09:
                    bundle.putInt("glass_num", 8);
                    break;
                case R.id.glass_10:
                    bundle.putInt("glass_num", 9);
                    break;
                case R.id.glass_11:
                    bundle.putInt("glass_num", 10);
                    break;
                case R.id.glass_12:
                    bundle.putInt("glass_num", 11);
                    break;
                case R.id.glass_13:
                    bundle.putInt("glass_num", 12);
                    break;
                case R.id.glass_14:
                    bundle.putInt("glass_num", 13);
                    break;
                case R.id.glass_15:
                    bundle.putInt("glass_num", 14);
                    break;
                default:
                    break;
            }

            intent.putExtras(bundle);
            startActivity(intent);
            // finish();  //为了返回 不用关闭
        }
    };

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

    protected void hideBottomUIMenu() {
        //隐藏虚拟按键，并且全屏
        if (Build.VERSION.SDK_INT > 11 && Build.VERSION.SDK_INT < 19) { // lower api
            View v = this.getWindow().getDecorView();
            v.setSystemUiVisibility(View.GONE);
        } else if (Build.VERSION.SDK_INT >= 19) {
            //for new api versions.
            View decorView = getWindow().getDecorView();
            int uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_FULLSCREEN;
            decorView.setSystemUiVisibility(uiOptions);
        }
    }


    //Byte 转 char 数组
    public static char byteToChar(byte[] b) {
        char c = (char) (((b[0] & 0xFF) << 8) | (b[1] & 0xFF));
        return c;
    }
}
