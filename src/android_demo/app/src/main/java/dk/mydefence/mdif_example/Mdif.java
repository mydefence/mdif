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
 * Program/file : Mdif.java                                                    *
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

import android.content.Context;
import android.util.Log;

import dk.mydefence.mdif_example.hdlc.Hdlc;
import dk.mydefence.mdif_example.hdlc.IHdlc;

public class Mdif {
    private static final String TAG = "MD Mdif";
    private final Hdlc mHdlc;
    private final MdifCore mCore;
    private final IMdif mCallback;
    private final MdifRfs mRfs;

    protected enum MdifStatus {
        DECODE_SUCCESS,
        DECODE_ERR_NO_PAYLOAD,
        DECODE_ERR_NO_DECODER,
        DECODE_ERR_OTHER
    }

    public Mdif(Context applicationContext, IMdif mdifCallback, IMdifCore mdifCoreCallback, IMdifRfs rfsCallback) {
        mCallback = mdifCallback;
        mHdlc = new Hdlc(applicationContext, mHdlcCallback);
        mCore = new MdifCore(mHdlc, mdifCoreCallback);
        mRfs = new MdifRfs(mHdlc, rfsCallback);
    }

    private final IHdlc mHdlcCallback = new IHdlc() {
        @Override
        public void onHdlcFrame(byte[] frame) {
            if (mCore.decode(frame) != MdifStatus.DECODE_SUCCESS &&
                    mRfs.decode(frame) != MdifStatus.DECODE_SUCCESS) {
                Log.w(TAG, "Unable to decode received HDLC message");
            }
        }

        @Override
        public void onHdlcConnected() {
            Log.d(TAG, "HDLC connected - requesting device info");
            mCore.sendDeviceInfoReq();
            mCallback.onConnected();
        }

        @Override
        public void onHdlcDisconnected() {
            mCallback.onDisconnected();
        }

        @Override
        public void onHdlcConnecting() {
            mCallback.onConnecting();
        }

        @Override
        public void onHdlcFrameSent(byte[] frame) {
            Log.d(TAG, "Message sent through HDLC");
            mCallback.onMessageSent(frame);
        }
    };


    public void onResume() {
        mHdlc.onResume();
    }

    public void onPause() {
        mHdlc.onPause();
    }

    public void onDestroy() {
        mHdlc.onDestroy();
    }

}
