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
 * Program/file : MdifRfs.java                                                 *
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

import dk.mydefence.mdif.rfs.Rfs;
import dk.mydefence.mdif_example.hdlc.Hdlc;

public class MdifRfs {
    private static final String TAG = "MD MdifRfs";
    private final Hdlc mHdlc;
    private final IMdifRfs mCallback;

    MdifRfs(Hdlc hdlc, IMdifRfs callback) {
        mCallback = callback;
        mHdlc = hdlc;
    }

    public Mdif.MdifStatus decode(byte[] buffer) {
        try {
            Rfs.RfsMsg rfs = Rfs.RfsMsg.parseFrom(buffer);
            Rfs.RfsMsg.MsgCase value = rfs.getMsgCase();
            Log.d(TAG, "I got a decoded message with value " + value + " (" + value.getNumber() + ")");
            switch (value) {
                case GET_DRONE_INFO_RES:
                    mCallback.onGetDroneInfoRes(rfs.getGetDroneInfoRes());
                    break;
                case STOP_RES:
                    mCallback.onGetStopRes(rfs.getStopRes());
                    break;
                case RFS_THREAT_IND:
                    mCallback.onGetRfsThreatInd(rfs.getRfsThreatInd());
                    break;
                case THREAT_STOPPED_IND:
                    mCallback.onGetThreatStoppedInd(rfs.getThreatStoppedInd());
                    break;
                case WIFI_THREAT_IND:
                    mCallback.onGetWifiThreatInd(rfs.getWifiThreatInd());
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
}
