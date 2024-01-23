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
 * Program/file : MdifCore.java                                                *
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

import android.util.Log;

import com.google.protobuf.InvalidProtocolBufferException;

import dk.mydefence.mdif.core.Core;
import dk.mydefence.mdif_example.hdlc.Hdlc;

public class MdifCore {

    private static final String TAG = "MD MdifCore";
    private final Hdlc mHdlc;
    private final IMdifCore mCallback;


    MdifCore(Hdlc hdlc, IMdifCore callback) {
        mCallback = callback;
        mHdlc = hdlc;
    }

    public Mdif.MdifStatus decode(byte[] buffer) {
        try {
            Core.CoreMsg core = Core.CoreMsg.parseFrom(buffer);
            Core.CoreMsg.MsgCase value = core.getMsgCase();
            Log.d(TAG, "I got a decoded message with value " + value + " (" + value.getNumber() + ")");
            switch (value) {
                case API_ERROR_IND:
                    Core.ApiErrorInd error = core.getApiErrorInd();
                    Log.d(TAG, "Got API_ERROR_IND: " + error.getError());
                    mCallback.onApiErrorInd(error);
                    break;
                case GET_DEVICE_INFO_RES:
                    Core.GetDeviceInfoRes info = core.getGetDeviceInfoRes();
                    Log.i(TAG, "GET_DEVICE_INFO_RES:");
                    Log.i(TAG, "\tDeviceType:          " + info.getDeviceType());
                    Log.i(TAG, "\tSerial number:       " + info.getSerialNumber());
                    Log.i(TAG, "\tDeviceType version:  " + info.getDeviceVersion());
                    Log.i(TAG, "\tSW version:          " + info.getSwVersion());
                    mCallback.onGetDeviceInfoRes(info);
                    break;
                case PING_RES:
                    mCallback.onGetPingRes(core.getPingRes());
                    break;
                case GET_BATTERY_STATUS_RES:
                    mCallback.onGetBatteryStatusRes(core.getGetBatteryStatusRes());
                    break;
                case RESET_RES:
                    mCallback.onResetRes(core.getResetRes());
                    break;
                case SET_IP_CONFIG_RES:
                    mCallback.onSetIpConfigRes(core.getSetIpConfigRes());
                    break;
                default:
                    Log.d(TAG, "Unknown MDIF message received: " + value + " (" + value.getNumber() + ")");
                    return Mdif.MdifStatus.DECODE_ERR_NO_DECODER;
            }
            return Mdif.MdifStatus.DECODE_SUCCESS;
        } catch (InvalidProtocolBufferException e) {
            throw new RuntimeException(e);
        }
    }

    public void sendDeviceInfoReq() {
        // Generate GetDeviceInfoReq
        Core.GetDeviceInfoReq.Builder req = Core.GetDeviceInfoReq.newBuilder();
        // Wrap it with CoreMsg
        Core.CoreMsg.Builder core = Core.CoreMsg.newBuilder();
        core.setGetDeviceInfoReq(req);
        byte[] frame = core.build().toByteArray();

        // Get byte array
        Log.d(TAG, "Sending frame of length: " + frame.length);
        // Send it
        mHdlc.hdlc_send_frame(frame, frame.length);
    }

}
