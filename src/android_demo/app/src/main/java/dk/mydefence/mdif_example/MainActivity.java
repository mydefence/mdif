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
 * Program/file : MainActivity.java                                            *
 *                                                                             *
 * Description  : Android implementation of MDIF                               *
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
package dk.mydefence.mdif_example;

import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import dk.mydefence.mdif.core.Core;
import dk.mydefence.mdif.rfs.Rfs;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MD Main";
    TextView textView;
    TextView statusView;
    TextView alertsView;
    Mdif mMdif;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // Get TextViews from layout
        textView = findViewById(R.id.mdifExampleMsg);
        statusView = findViewById(R.id.statusMsg);
        alertsView = findViewById(R.id.alerts);

        // Create instance of Mdif class. This is what kicks it all off starting
        // USB service and connecting to Hdlc using JNI.
        mMdif = new Mdif(getApplicationContext(), mMdifCallback, mMdifCoreCallback, mRfsCallback);
    }

    IMdif mMdifCallback = new IMdif() {
        @Override
        public void onConnected() {
            Log.d(TAG, "HDLC connected");
            updateTextViewOnUiThread(statusView, "Connected");
        }

        @Override
        public void onDisconnected() {
            Log.d(TAG, "HDLC disconnected");
            updateTextViewOnUiThread(statusView, "Disconnected");
            updateTextViewOnUiThread(textView, "");
        }

        @Override
        public void onConnecting() {
            Log.d(TAG, "HDLC connecting");
            updateTextViewOnUiThread(statusView, "Connecting...");
        }

        @Override
        public void onMessageSent(byte[] frame) {
            Log.d(TAG, "HDLC message sent");
        }
    };

    private final IMdifCore mMdifCoreCallback = new IMdifCore() {
        @Override
        public void onGetDeviceInfoRes(Core.GetDeviceInfoRes getDeviceInfoRes) {
            String sb = "Device type:       " + getDeviceInfoRes.getDeviceType() +
                    "\nSW version:        " + getDeviceInfoRes.getSwVersion() +
                    "\nSerial number:   " + getDeviceInfoRes.getSerialNumber() +
                    "\nDevice version:  " + getDeviceInfoRes.getDeviceVersion();
            updateTextViewOnUiThread(textView, sb);
        }

        @Override
        public void onApiErrorInd(Core.ApiErrorInd error) {
            updateTextViewOnUiThread(textView, error.toString());
        }

        @Override
        public void onGetPingRes(Core.PingRes pingRes) {

        }

        @Override
        public void onGetBatteryStatusRes(Core.GetBatteryStatusRes getBatteryStatusRes) {

        }

        @Override
        public void onResetRes(Core.ResetRes resetRes) {

        }

        @Override
        public void onSetIpConfigRes(Core.SetIpConfigRes setIpConfigRes) {

        }
    };

    private final IMdifRfs mRfsCallback = new IMdifRfs() {
        @Override
        public void onGetWifiThreatInd(Rfs.WifiThreatInd wifiThreatInd) {
            updateTextViewOnUiThread(alertsView, "WIFI Threat Ind\n\n" + wifiThreatInd.toString());
        }

        @Override
        public void onGetThreatStoppedInd(Rfs.ThreatStoppedInd threatStoppedInd) {
            updateTextViewOnUiThread(alertsView, "Threat Stopped Ind\n\n" + threatStoppedInd.toString());
        }

        @Override
        public void onGetRfsThreatInd(Rfs.RfsThreatInd rfsThreatInd) {
            updateTextViewOnUiThread(alertsView, "Threat Ind\n\n" + rfsThreatInd.toString());
        }

        @Override
        public void onGetStopRes(Rfs.StopRes stopRes) {

        }

        @Override
        public void onGetDroneInfoRes(Rfs.GetDroneInfoRes getDroneInfoRes) {

        }
    };

    private void updateTextViewOnUiThread(TextView view, String text) {
        runOnUiThread(() -> view.setText(text));
    }


    @Override
    public void onResume() {
        super.onResume();
        mMdif.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
        mMdif.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mMdif.onDestroy();
    }

    // Load JNI library
    static {
        System.loadLibrary("hdlc-lib");
    }

}
