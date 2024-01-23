/*******************************************************************************
 *                                                                             *
 *                                                 ,,                          *
 *                                                       ,,,,,                 *
 *                                                           ,,,,,             *
 *           ,,,,,,,,,,,,,,,,,,,,,,,,,,,,                        ,,,,          *
 *          ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,            ,,,,          ,,,,        *
 *          ,,,,,       ,,,,,      ,,,,,,                ,,,,        ,,,       *
 *          ,,,,,       ,,,,,      ,,,,,,                   ,,,        ,,,     *
 *          ,,,,,       ,,,,,      ,,,,,,       ,,,           ,,,        ,     *
 *          ,,,,,       ,,,,,      ,,,,,,           ,,,         ,,        ,    *
 *          ,,,,,       ,,,,,      ,,,,,,              ,,        ,,            *
 *          ,,,,,       ,,,,,      ,,,,,,                ,        ,            *
 *          ,,,,,       ,,,,,      ,,,,,,                 ,                    *
 *          ,,,,,       ,,,,,      ,,,,,,                                      *
 *          ,,,,,       ,,,,,      ,,,,,,                                      *
 *                                       ,,,,,,,,,,,,,,,,,,,,,,,,,,            *
 *                                       ,,,,,,,,,,,,,,,,,,,,,,,,,,,,          *
 *                                       ,,,,,                  ,,,,,,         *
 *                     ,                 ,,,,,                  ,,,,,,         *
 *             ,        ,,               ,,,,,                  ,,,,,,         *
 *    ,        ,,        ,,,             ,,,,,                  ,,,,,,         *
 *     ,        ,,,         ,,,          ,,,,,                  ,,,,,,         *
 *     ,,,       ,,,                     ,,,,,                  ,,,,,,         *
 *      ,,,        ,,,,                  ,,,,,                  ,,,,,,         *
 *        ,,,         ,,,,               ,,,,,                  ,,,,,,         *
 *         ,,,,,            ,,,,         ,,,,,,,,,,,,,,,,,,,,,,,,,,,,          *
 *            ,,,,                       ,,,,,,,,,,,,,,,,,,,,,,,,,,            *
 *               ,,,,,                                                         *
 *                    ,,,,,                                                    *
 *                                                                             *
 * Program/file : Hdlc.java                                                    *
 *                                                                             *
 * Description  : Android implementation of OS abstraction interface for hdlc  *
 *              :                                                              *
 *                                                                             *
 * Copyright 2023 MyDefence A/S.                                               *
 *                                                                             *
 * Licensed under the Apache License, Version 2.0 (the "License");             *
 * you may not use this file except in compliance with the License.            *
 * You may obtain a copy of the License at                                     *
 *                                                                             *
 * http://www.apache.org/licenses/LICENSE-2.0                                  *
 *                                                                             *
 * Unless required by applicable law or agreed to in writing, software         *
 * distributed under the License is distributed on an "AS IS" BASIS,           *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.    *
 * See the License for the specific language governing permissions and         *
 * limitations under the License.                                              *
 *                                                                             *
 *                                                                             *
 *                                                                             *
 *******************************************************************************/
package dk.mydefence.mdif_example.hdlc;

import android.content.Context;
import android.util.Log;

import androidx.annotation.Keep;

import dk.mydefence.mdif_example.serial.IUsbOnData;
import dk.mydefence.mdif_example.serial.UsbCommunication;

/**
 * Processing USB data into HDLC frames which are then passed on
 * to MDIF.
 * <p>
 * This class function as a connection between USB and HDLC.
 */
public class Hdlc {
    private static final String TAG = "MD HDLC Impl";

    private static UsbCommunication mUsbCom;
    private static IHdlc mCallback = null;

    public Hdlc(Context context, IHdlc callback) {
        mCallback = callback;
        mUsbCom = new UsbCommunication(context, mCallbackUsb);
        initializeHdlc();
        mCallback.onHdlcConnecting();
    }

    /**
     * Raw data received on USB. Pass it on to HDLC
     */
    IUsbOnData mCallbackUsb = new IUsbOnData() {
        @Override
        public void onData(byte[] bytes) {
            hdlc_os_rx(bytes, bytes.length);
        }
    };

    public void onResume() {
        mUsbCom.onResume();
    }

    public void onPause() {
        mUsbCom.onPause();
    }

    public void onDestroy() {
        mUsbCom.onDestroy();
    }

    // ------------------------------------------
    // JNI interfaces
    // ------------------------------------------

    @Keep
    static void hdlc_frame_sent_cb(byte[] frame, int len) {
        Log.d(TAG, "hdlc_frame_sent_cb. " + len + " bytes or " + frame.length + " as byte array length");
        mCallback.onHdlcFrameSent(frame);
    }

    @Keep
    static void hdlc_recv_frame_cb(byte[] frame, int len) {
        Log.d(TAG, "hdlc_recv_frame_cb. " + len + " bytes or " + frame.length + " as byte array length");
        mCallback.onHdlcFrame(frame);
    }

    @Keep
    static void hdlc_connected_cb() {
        Log.d(TAG, "hdlc_connected_cb");
        mCallback.onHdlcConnected();
    }

    public native void hdlc_send_frame(byte[] frame, int len);

    @Keep
    static void hdlc_reset_cb() {
        Log.d(TAG, "hdlc_reset_cb");
        mCallback.onHdlcDisconnected();
    }


    /**
     * JNI call to initialize HDLC
     */
    native void initializeHdlc();

    /**
     * JNI callback when package has been received through HDLC
     * <p>
     * The package is sent to MDIF to be processed
     *
     * @param buffer
     */
    @Keep
    static void hdlc_os_tx(byte[] buffer) {
        Log.d(TAG, "hdlc_os_tx");
        mUsbCom.write(buffer);
    }

    /**
     * Called by integration when new raw serial data is received.
     *
     * @param buffer byte array holding received bytes
     * @param count  number of bytes in array
     */
    @Keep
    native void hdlc_os_rx(byte[] buffer, int count);

}
