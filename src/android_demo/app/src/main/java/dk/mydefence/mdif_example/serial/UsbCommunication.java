package dk.mydefence.mdif_example.serial;

import static dk.mydefence.mdif_example.serial.UsbService.MSG_EXTRA_PARCELABLE;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;


public class UsbCommunication {
    private static final String TAG = "MD Communication";
    private final Context mContext;
    static IUsbOnData mUsbCallback = null;
    private boolean mReceiversRegistered = false;
    private Messenger mServiceMessenger = null;
    private final Messenger mReceiveMessenger = new Messenger(new MainHandler());
    private boolean mUsbConnected = false;

    /**
     * Notifications from UsbService will be received here.
     */
    private final BroadcastReceiver usbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action != null) {
                switch (action) {
                    case UsbService.ACTION_USB_PERMISSION_GRANTED:
                        Toast.makeText(mContext, "USB Permission granted", Toast.LENGTH_SHORT).show();
                        Log.d(TAG, "USB ready");
                        mUsbConnected = true;
                        break;
                    case UsbService.ACTION_USB_PERMISSION_NOT_GRANTED:
                        Toast.makeText(mContext, "USB Permission not granted", Toast.LENGTH_SHORT).show();
                        Log.d(TAG, "USB Permission not granted");
                        mUsbConnected = false;
                        break;
                    case UsbService.ACTION_NO_USB:
                        Toast.makeText(mContext, "No USB connected", Toast.LENGTH_SHORT).show();
                        Log.d(TAG, "No USB connected");
                        mUsbConnected = false;
                        break;
                    case UsbService.ACTION_USB_DISCONNECTED:
                        Log.d(TAG, "USB disconnected");
                        if (mUsbConnected){
                            Toast.makeText(mContext, "USB disconnected", Toast.LENGTH_SHORT).show();
                        }
                        mUsbConnected = false;
                        break;
                    case UsbService.ACTION_USB_NOT_SUPPORTED:
                        Toast.makeText(mContext, "USB device not supported", Toast.LENGTH_SHORT).show();
                        Log.d(TAG, "USB device not supported");
                        mUsbConnected = false;
                        break;
                    case UsbService.ACTION_USB_DEVICE_NOT_WORKING:
                        Toast.makeText(mContext, "Error in USB - restarting", Toast.LENGTH_SHORT).show();
                        Log.d(TAG, "Error in USB - restarting");
                        mUsbConnected = false;

                        mContext.unbindService(mUsbConnection);
                        startService(UsbService.class, mUsbConnection); // Start UsbService (if it was not started before) and bind it.
                        break;

                    default:
                        Log.w(TAG, "Received action " + action + " is not handled");
                }
            }
        }
    };

    private final ServiceConnection mUsbConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName componentName, IBinder binder) {
            mServiceMessenger = new Messenger(binder);
            Message msg = Message.obtain();
            msg.replyTo = mReceiveMessenger;
            try {
                Log.d(TAG, "onServiceConnected - sending messenger");
                mServiceMessenger.send(msg);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
            Log.d(TAG, "onServiceConnected done");
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            Log.d(TAG, "onServiceDisconnected");
            mServiceMessenger = null;
        }
    };

    public UsbCommunication(Context context, IUsbOnData callBack) {
        mContext = context;
        mUsbCallback = callBack;
        setFilters();  // Start listening for notifications from UsbService.
        startService(UsbService.class, mUsbConnection); // Start UsbService (if it was not started before) and bind it.
    }

    /**
     * Write byte array to USB serial
     *
     * @param array data to be written
     */
    public void write(byte[] array) {
        if (mUsbConnected) {
            Bundle bundle = new Bundle();
            bundle.putByteArray(MSG_EXTRA_PARCELABLE, array);
            sendMessage(UsbService.DATA_SEND, bundle);
        } else {
            Log.w(TAG, "Discarding message as we are not connected yet");
        }
    }

    /**
     * Listen for intents sent from UsbService
     */
    private void setFilters() {
        if (!mReceiversRegistered) {
            IntentFilter filter = new IntentFilter();
            filter.addAction(UsbService.ACTION_USB_PERMISSION_GRANTED);
            filter.addAction(UsbService.ACTION_NO_USB);
            filter.addAction(UsbService.ACTION_USB_DISCONNECTED);
            filter.addAction(UsbService.ACTION_USB_NOT_SUPPORTED);
            filter.addAction(UsbService.ACTION_USB_PERMISSION_NOT_GRANTED);
            filter.addAction(UsbService.ACTION_USB_DEVICE_NOT_WORKING);
            mContext.registerReceiver(usbReceiver, filter);
            mReceiversRegistered = true;
        }
    }

    /**
     * Bind and create UsbService
     *
     * @param service
     * @param serviceConnection
     */
    private void startService(Class<?> service, ServiceConnection serviceConnection) {
        Intent bindingIntent = new Intent(mContext, service);
        mContext.bindService(bindingIntent, serviceConnection, Context.BIND_AUTO_CREATE);
    }

    /**
     * Send messages to UsbService
     *
     * @param what
     * @param bundle
     */
    private void sendMessage(int what, Bundle bundle) {
        Message msg = Message.obtain();
        msg.replyTo = mReceiveMessenger;
        msg.what = what;
        msg.setData(bundle);
        try {
            mServiceMessenger.send(msg);
        } catch (RemoteException | NullPointerException e) {
            e.printStackTrace();
        }
    }

    public void onResume() {
        Log.d(TAG, "onResume");
    }

    public void onPause() {
        Log.d(TAG, "onPause");
    }

    public void onDestroy() {
        Log.d(TAG, "onDestroy");
        if (mContext == null) {
            return;
        }
        if (mReceiversRegistered) {
            mReceiversRegistered = false;
            mContext.unregisterReceiver(usbReceiver);
        }
        if (mServiceMessenger != null) {
            // This will stop the USB Service
            mContext.unbindService(mUsbConnection);
        }
    }

    /**
     * Receive messages from UsbService
     */
    private static class MainHandler extends Handler {

        MainHandler() {
            super(Looper.getMainLooper());
        }
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case UsbService.CTS_CHANGE:
                    Log.d(TAG, "CTS_CHANGE");
                    break;
                case UsbService.DSR_CHANGE:
                    Log.d(TAG, "DSR_CHANGE");
                    break;
                case UsbService.DATA_RECEIVED:
                    if (mUsbCallback != null) {
                        mUsbCallback.onData(msg.getData().getByteArray(MSG_EXTRA_PARCELABLE));
                    } else {
                        Log.d(TAG, "No callback defined for data");
                    }
                    break;
                case UsbService.USB_CONNECTED:
                    Log.d(TAG, "USB service connected");
                    break;
            }
        }
    }
}
