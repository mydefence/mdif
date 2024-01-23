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
 * Program/file : hdlc_jni_wrapper.c                                           *
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
#include <jni.h>
#include <hdlc_jni.h>

// This file serves as a wrapper between JNI function naming which is dependent
// on project structure, and the generic Java implementation which can be found
// in src/hdlc/ports/java/hdlc_jni.c

/**
 * Java calls into this function to initialize HDLC
 *
 * Wrapper from project dependent JNI function naming to generic implementation
 * in in src/hdlc/ports/java/hdlc_jni.c
 *
 * @param env JNIEnv pointer
 * @param thiz Java object
 * @return JNIEXPORT
 */
JNIEXPORT void JNICALL
Java_dk_mydefence_mdif_1example_hdlc_Hdlc_initializeHdlc(JNIEnv *env, jobject thiz) {
    initialize_hdlc_wrapper(env, thiz);
}

/**
 * Data has been received by serial device. Data is passed to HDLC from Java.
 *
 * Wrapper from project dependent JNI function naming to generic implementation
 * in in src/hdlc/ports/java/hdlc_jni.c
 *
 * @param env JNIEnv pointer
 * @param thiz Java object
 * @param buffer byte array received from serial
 * @param count number of bytes in buffer
 */
JNIEXPORT void JNICALL
Java_dk_mydefence_mdif_1example_hdlc_Hdlc_hdlc_1os_1rx(JNIEnv *env, jobject thiz, jbyteArray buffer,
                                                       jint count) {
    hdlc_os_rx_wrapper(env, thiz, buffer, count);
}

/**
 * Java is sending a frame of data through HDLC.
 *
 * Once the data has been handed over to HDCL it will be processed and eventually
 * callbacks to hdlc_os_tx() will pass the raw data to the serial line.
 *
 * Wrapper from project dependent JNI function naming to generic implementation
 * in in src/hdlc/ports/java/hdlc_jni.c
 *
 * @param env JNIEnv pointer
 * @param thiz Java object
 * @param frame byte array to be sent using HDLC
 * @param len number of bytes in frame
 */
JNIEXPORT void JNICALL
Java_dk_mydefence_mdif_1example_hdlc_Hdlc_hdlc_1send_1frame(JNIEnv *env, jobject thiz,
                                                            jbyteArray frame,
                                                            jint len) {
    hdlc_send_frame_wrapper(env, thiz, frame, len);
}
