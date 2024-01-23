package dk.mydefence.mdif_example.serial;

import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

import java.io.IOException;
import java.io.Serializable;
import java.util.List;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

public class UsbService extends Service {
    private static final String TAG = "MD USB";

    static final String ACTION_USB_ATTACHED = "android.hardware.usb.action.USB_DEVICE_ATTACHED";
    static final String ACTION_USB_DETACHED = "android.hardware.usb.action.USB_DEVICE_DETACHED";

    static final String ACTION_USB_PERMISSION = "dk.mydefence.mdif_example.serial.usbservice.USB_PERMISSION";
    static final String ACTION_NO_USB = "dk.mydefence.mdif_example.serial.usbservice.NO_USB";
    static final String ACTION_USB_PERMISSION_GRANTED = "dk.mydefence.mdif_example.serial.usbservice.USB_PERMISSION_GRANTED";
    static final String ACTION_USB_PERMISSION_NOT_GRANTED = "dk.mydefence.mdif_example.serial.usbservice.USB_PERMISSION_NOT_GRANTED";
    static final String ACTION_USB_DISCONNECTED = "dk.mydefence.mdif_example.serial.usbservice.USB_DISCONNECTED";
    static final String ACTION_USB_READY = "dk.mydefence.mdif_example.serial.usbservice.USB_READY";
    static final String ACTION_CDC_DRIVER_NOT_WORKING = "dk.mydefence.mdif_example.serial.usbservice.ACTION_CDC_DRIVER_NOT_WORKING";
    static final String ACTION_USB_DEVICE_NOT_WORKING = "dk.mydefence.mdif_example.serial.usbservice.ACTION_USB_DEVICE_NOT_WORKING";
    static final String ACTION_USB_NOT_SUPPORTED = "dk.mydefence.mdif_example.serial.usbservice.USB_NOT_SUPPORTED";
    static final String MSG_EXTRA_PARCELABLE = "parcelable";
    // Messages
    public static final int CTS_CHANGE = 0;
    public static final int DSR_CHANGE = 1;
    public static final int DATA_RECEIVED = 2;
    public static final int USB_CONNECTED = 3;
    public static final int DATA_SEND = 6;
    public static final int SERVICE_CONNECTED = 7;
    public static final int DEFAULT_BAUD_RATE = 460800;


    private Handler mHandler = new Handler(Looper.getMainLooper());
    private Context mContext;
    private final Messenger myMessenger = new Messenger(new IncomingHandler());
    private static Messenger mClientMessenger = null;
    private boolean mSerialPortConnected;
    private boolean mServiceConnected = false;
    private UsbManager mUsbManager;
    private UsbDevice mUsbDevice;
    private UsbSerialDriver mUsbDriver;
    private static UsbSerialPort mUsbSerialPort;
    private UsbDeviceConnection mUsbConnection;
    private static SerialInputOutputManager usbIoManager;

    /*
     * Different notifications from OS will be received here.
     */
    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action != null) {
                switch (action) {
                    case ACTION_USB_PERMISSION:
                        Bundle extras = intent.getExtras();
                        if (extras != null) {
                            boolean granted = extras.getBoolean(UsbManager.EXTRA_PERMISSION_GRANTED);
                            if (granted) { // User accepted our USB mUsbConnection. Try to open the mUsbDevice as a serial port.
                                Intent intentGranted = new Intent(ACTION_USB_PERMISSION_GRANTED);
                                context.sendBroadcast(intentGranted);
                                mUsbConnection = mUsbManager.openDevice(mUsbDevice);
                                Log.d(TAG, "Initializing Connection Thread");
                                new ConnectionThread().start();
                            } else { // User did not accepted our USB mUsbConnection. Send an Intent to the Main Activity.
                                Intent intentNotGranted = new Intent(ACTION_USB_PERMISSION_NOT_GRANTED);
                                context.sendBroadcast(intentNotGranted);
                            }
                        }
                        break;
                    case ACTION_USB_ATTACHED:
                        if (!mSerialPortConnected) {
                            findSerialPortDevice(); // A USB mUsbDevice has been attached. Try to open it as a serial port.
                        }
                        break;
                    case ACTION_USB_DETACHED: // USB mUsbDevice was disconnected. Send an Intent to the Main Activity.
                        Intent intentDisconnected = new Intent(ACTION_USB_DISCONNECTED);
                        context.sendBroadcast(intentDisconnected);
                        mSerialPortConnected = false;
                        break;
                    default:
                        Log.w(TAG, "Unknown action " + action + " received");
                }
            }
        }
    };

    /**
     * Handle data from SerialForAndroid
     */
    private final SerialInputOutputManager.Listener readCallback = new SerialInputOutputManager.Listener() {
        @Override
        public void onNewData(byte[] bytes) {
            sendMessage(UsbService.DATA_RECEIVED, bytes);
        }

        @Override
        public void onRunError(Exception e) {
            Log.d(TAG, "Run error serial!");
            Intent intent = new Intent(ACTION_USB_DEVICE_NOT_WORKING);
            mContext.sendBroadcast(intent);
        }
    };

    @Override
    public void onCreate() {
        this.mContext = this;

        Log.v(TAG, "onCreate");
        mSerialPortConnected = false;
        mServiceConnected = true;
        setFilter();
        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        findSerialPortDevice();
    }

    private void findSerialPortDevice() {
        // Find all available drivers from attached devices.
        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        if (manager == null) {
            Log.e(TAG, "Unable to get system USB service");
            Log.e(TAG, "Retry USB service in a few seconds");
            mHandler.postDelayed(this::findSerialPortDevice, 5000);
            return;
        }
        List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);
        if (availableDrivers.isEmpty()) {
            // There are no USB devices connected. Send an Intent to MainActivity.
            Intent intent = new Intent(ACTION_NO_USB);
            sendBroadcast(intent);
            return;
        }
        boolean keep = true;
        for (UsbSerialDriver driver : availableDrivers) {
            mUsbDriver = driver;
            mUsbDevice = mUsbDriver.getDevice();
            int deviceVID = mUsbDevice.getVendorId();
            int devicePID = mUsbDevice.getProductId();

            // Filter out some devices we have seen on different systems
            if (deviceVID != 0x1d6b && (devicePID != 0x0001 && devicePID != 0x0002 && devicePID != 0x0003) && deviceVID != 0x5c6 && devicePID != 0x904c) {
                // There is a mUsbDevice connected to our Android mUsbDevice. Try to open it as a serial port.
                Log.d(TAG, String.format("Request user permission PID 0x%04X VID 0x%04X", devicePID, deviceVID));
                requestUserPermission();
                keep = false;
            } else {
                mUsbConnection = null;
                mUsbDevice = null;
                mUsbDriver = null;
            }

            if (!keep) {
                break;
            }
        }
        if (!keep) {
            // There is no USB devices connected (but USB host were listed). Send an Intent to MainActivity.
            Intent intent = new Intent(ACTION_NO_USB);
            sendBroadcast(intent);
        }
    }

    /**
     * Transmit message on serial line
     *
     * @param message byte[] message to be transmitted
     */
    private static void write(byte[] message) {
        // Log.d(TAG, "Writing message to serial");
        usbIoManager.writeAsync(message);
    }

    private void setFilter() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(ACTION_USB_PERMISSION);
        filter.addAction(ACTION_USB_DETACHED);
        filter.addAction(ACTION_USB_ATTACHED);

        registerReceiver(mUsbReceiver, filter);
    }

    void sendMessage(int what, Serializable serializable) {
        Bundle bundle = new Bundle();
        bundle.putSerializable(MSG_EXTRA_PARCELABLE, serializable);
        sendMessage(what, bundle);
    }

    static void sendMessage(int what, Bundle bundle) {
        Message msg = Message.obtain();
        if (bundle != null) {
            msg.setData(bundle);
        }
        sendMessage(what, msg);
    }

    private static void sendMessage(int what, Message msg) {
        if (mClientMessenger == null) {
            Log.d(TAG, "Trying to send a message but I do not know who to send it to");
            return;
        }
        msg.what = what;
        try {
            mClientMessenger.send(msg);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    /*
     * Request user permission. The response will be received in the BroadcastReceiver.
     */
    private void requestUserPermission() {
        PendingIntent mPendingIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION),
                PendingIntent.FLAG_MUTABLE);
        mUsbManager.requestPermission(mUsbDevice, mPendingIntent);
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.v(TAG, "OnBind - return binder");
        return myMessenger.getBinder();
    }

    @Override
    public boolean onUnbind(Intent intent) {
        Log.v(TAG, "onUnbind");
        unregisterReceiver(mUsbReceiver);
        mServiceConnected = false;

        // Something has happened with the other end - close down service
        // We only have one binding. If that is removed we assume we will get
        // another start-service command when we should start again
        stopSelf();
        return super.onUnbind(intent);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.v(TAG, "onStartCommand");
        return Service.START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.v(TAG, "onDestroy");
        // Unregister receivers
        if (mServiceConnected) {
            unregisterReceiver(mUsbReceiver);
            mServiceConnected = false;
        }
        // Reset messenger
        mClientMessenger = null;

        // Close USB serial port
        try {
            if (mUsbSerialPort != null) {
                mUsbSerialPort.close();
                mUsbSerialPort = null;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        // Close USB IO Manager thread
        if (usbIoManager != null) {
            usbIoManager.stop();
            usbIoManager = null;
        }

        // Close USB connection
        if (mUsbConnection != null) {
            mUsbConnection.close();
            mUsbConnection = null;
        }
    }

    /*
     * A simple thread to open a serial port.
     */
    private class ConnectionThread extends Thread {
        @Override
        public void run() {
            Log.v(TAG, "Running connection thread");
            mUsbSerialPort = mUsbDriver.getPorts().get(0);
            Log.d(TAG, "Serial port is: " + mUsbSerialPort);
            if (mUsbSerialPort != null) {
                try {
                    mUsbSerialPort.open(mUsbConnection);
                    Log.d(TAG, "Serial port open");
                    mUsbSerialPort.setParameters(DEFAULT_BAUD_RATE, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
                    usbIoManager = new SerialInputOutputManager(mUsbSerialPort, readCallback);
                    usbIoManager.setReadTimeout(1);
                    usbIoManager.run();
                    mSerialPortConnected = true;

                    // Send an Intent to Main Activity
                    Intent intent = new Intent(ACTION_USB_READY);
                    mContext.sendBroadcast(intent);
                } catch (IOException e) {
                    // Serial port could not be opened, maybe an I/O error or if CDC driver was chosen.
                    // Send an Intent to Main Activity
                    e.printStackTrace();
                    Log.e(TAG, "Serial port failure");
                    Intent intent = new Intent(ACTION_USB_DEVICE_NOT_WORKING);
                    mContext.sendBroadcast(intent);
                }
            } else {
                // No driver for given mUsbDevice.
                Log.d(TAG, "USB not supported");
                Intent intent = new Intent(ACTION_USB_NOT_SUPPORTED);
                mContext.sendBroadcast(intent);
            }
            Log.v(TAG, "Connection thread has finished");
        }
    }

    /**
     * Messages sent to UsbService
     */
    static class IncomingHandler extends Handler {

        IncomingHandler() {
            super(Looper.getMainLooper());
        }

        @Override
        public void handleMessage(Message msg) {

            // Upon first message we will return a USB_CONNECTED message as a handshake
            if (mClientMessenger == null && msg.replyTo != null) {
                mClientMessenger = msg.replyTo;
                Message m = new Message();
                m.what = USB_CONNECTED;
                try {
                    mClientMessenger.send(m);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }

            // Incoming messages
            if (mClientMessenger != null) {
                Bundle bundle = msg.getData();
                switch (msg.what) {
                    case DATA_SEND:
                        byte[] data = bundle.getByteArray(MSG_EXTRA_PARCELABLE);
                        if (data == null) {
                            Log.e(TAG, "Message bundle contains no MSG_EXTRA_PARCELABLE");
                            break;
                        }
                        // Log.v(TAG, "Sending data: " + data.length + " bytes");
                        write(data);
                        break;
                    default:
                        break;
                }
            }
        }
    }
}
