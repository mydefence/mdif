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
 * Program/file : hdlc_jni.c                                                   *
 *                                                                             *
 * Description  : Java port (OS abstraction or integration) interface to       *
 *              : application.                                                 *
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
#include <assert.h>
#include <inttypes.h>
#include <jni.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "log/log.h"
#include "hdlc/include/hdlc.h"
#include "hdlc/include/hdlc_os.h"
#include "hdlc/ports/java/java_port.h"


static int counter_attach_current_thread = 0;

// processing callback to handler class
typedef struct jni_context {
    JavaVM *javaVM;
    jclass jniHdlcClz;
    jobject jniHdlcObj;
    jmethodID hdlc_os_tx_id;
    jmethodID hdlc_frame_sent_cb_id;
    jmethodID hdlc_recv_frame_cb_id;
    jmethodID hdlc_reset_cb_id;
    jmethodID hdlc_connected_cb_id;

} JniContext;
JniContext g_ctx;

jmethodID locate_method_id(JNIEnv *env, char *method_name, char *arguments) {
    jmethodID method_id = (*env)->GetStaticMethodID(env, g_ctx.jniHdlcClz, method_name, arguments);
    if (method_id == NULL) {
        log_error("Unable to locate method '%s'", method_name);
        return NULL;  // Unable to locate required method
    } else {
        log_debug("Method '%s' was found", method_name);
        return method_id;
    }
}

/*
 * processing one time initialization:
 *     Cache the javaVM into our context
 *     Find class ID for JniHelper
 *     Create an instance of JniHelper
 *     Make global reference since we are using them from a native thread
 * Note:
 *     All resources allocated here are never released by application
 *     we rely on system to free all global refs when it goes away;
 *     the pairing function JNI_OnUnload() never gets called at all.
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    memset(&g_ctx, 0, sizeof(g_ctx));

    g_ctx.javaVM = vm;
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;  // JNI version not supported.
    }

    jclass clz =
            (*env)->FindClass(env, "dk/mydefence/mdif_example/hdlc/Hdlc");
    g_ctx.jniHdlcClz = (*env)->NewGlobalRef(env, clz);

    jmethodID jniHdlcCtor =
            (*env)->GetMethodID(env, g_ctx.jniHdlcClz, "<init>", "()V");
    jobject handler = (*env)->NewObject(env, g_ctx.jniHdlcClz, jniHdlcCtor);
    g_ctx.jniHdlcObj = (*env)->NewGlobalRef(env, handler);


    // Finding callback methods
    g_ctx.hdlc_os_tx_id = locate_method_id(env, "hdlc_os_tx", "([B)V");
    g_ctx.hdlc_frame_sent_cb_id = locate_method_id(env, "hdlc_frame_sent_cb", "([BI)V");
    g_ctx.hdlc_recv_frame_cb_id = locate_method_id(env, "hdlc_recv_frame_cb", "([BI)V");
    g_ctx.hdlc_reset_cb_id = locate_method_id(env, "hdlc_reset_cb", "()V");
    g_ctx.hdlc_connected_cb_id = locate_method_id(env, "hdlc_connected_cb", "()V");

    if (g_ctx.hdlc_os_tx_id == 0 ||
        g_ctx.hdlc_frame_sent_cb_id == 0 ||
        g_ctx.hdlc_recv_frame_cb_id == 0 ||
        g_ctx.hdlc_reset_cb_id == 0 ||
        g_ctx.hdlc_connected_cb_id == 0) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}


//////////////////////////////////////////////////////////////////////////////
// Implementation of HDLC callbacks

/**
 * Attach current thread to the Java VM
 *
 * DetachCurrentThread must be called for each AttachCurrentThread but since this example only
 * communicate with Jave from a single thread then DetachCurrentThread can be omitted.
 *
 * @param env JNIEnv pointer
 */
void attatch_current_thread(JNIEnv **env) {
    jint res = (*g_ctx.javaVM)->GetEnv(g_ctx.javaVM, (void **) env, JNI_VERSION_1_6);
    if (res != JNI_OK) {
        res = (*g_ctx.javaVM)->AttachCurrentThread(g_ctx.javaVM, env, NULL);
        if (JNI_OK != res) {
            log_error("Failed to AttachCurrentThread, ErrorCode = %d", res);
            return;
        }
        // Make sure you deteach if using more than one thread
        if (++counter_attach_current_thread == 1) {
            log_info("Attaced to current thread");
            log_info("Note that if working with multiple threads then "
                 "DetachCurrentThread MUST be used to free resources.");
        } else {
            log_warn("Attaced to current thread: %d", counter_attach_current_thread);
            log_warn("Attached to current thread multiple times!");
            log_warn("You should call DetachCurrentThread() before attaching again");
        }
    }
}

/**
 * Callback function called when a frame has been sent, or otherwise discarded.
 *
 * @param _hdlc HDLC instance data allocated by hdlc_init()
 * @param frame parameter from hdlc_send_frame()
 * @param len parameter from hdlc_send_frame()
 */
void hdlc_frame_sent_cb(hdlc_data_t *_hdlc, const uint8_t *frame, uint32_t len) {
    // TX is only logged when acknowledged, because that was easy to implement
    if (1) {
        log_debug("hdlc transferred frame %d bytes\n", len);
    }
    // Connect to current thread if not already connected
    JNIEnv *env;
    attatch_current_thread(&env);

    // Building raw data array copy and call static method in Java
    jbyteArray rawDataCopy = (*env)->NewByteArray(env, len);
    (*env)->SetByteArrayRegion(env, rawDataCopy, 0, len, (const jbyte *) frame);
    (*env)->CallStaticVoidMethod(env, g_ctx.jniHdlcClz, g_ctx.hdlc_frame_sent_cb_id, rawDataCopy,
                                 len);
    (*env)->DeleteLocalRef(env, rawDataCopy);
    // Free data allocated in Java_dk_mydefence_mdif_1example_hdlc_Hdlc_hdlc_1send_1frame
    free((void *) frame);
}

/**
 * Callback function called when a frame (data or UI) has been received.
 *
 *  This function is called when a frame has been received. Frames are always
 *  delivered in the same order they are sent, with the exception that UI frames
 *  may overtake data frames.
 *
 *  @param _hdlc HDLC instance data allocated by hdlc_init()
 *  @param frame Pointer to a received HDLC frame. The pointer is invalid
 *  after this function returns.
 *  @param len Length of the frame
 */
void hdlc_recv_frame_cb(hdlc_data_t *_hdlc, uint8_t *frame, uint32_t len) {
    log_debug("hdlc recv frame %d bytes\n", len);

    // Connect to current thread if not already connected
    JNIEnv *env;
    attatch_current_thread(&env);

    // Building raw data array copy and call static method in Java
    jbyteArray rawDataCopy = (*env)->NewByteArray(env, len);
    (*env)->SetByteArrayRegion(env, rawDataCopy, 0, len, (const jbyte *) frame);
    (*env)->CallStaticVoidMethod(env, g_ctx.jniHdlcClz, g_ctx.hdlc_recv_frame_cb_id, rawDataCopy,
                                 len);
    (*env)->DeleteLocalRef(env, rawDataCopy);
}

/**
 * This is called when HDLC sequence number are reset, which happens either
 * 1. In case of retransmission timeout (see HDLC_RETRANSMIT_CNT in hdlc_os.h)
 * 2. If integration indicates loss of link or frees the HDLC instance
 * 3. If HDLC SABM frame is received from peer
 *
 * This is followed by cleanup of the current TX queue including calling
 * hdlc_frame_sent_cb(). hdlc_send_frame() and hdcl_send_frame_unacknowledged()
 * will fail until hdlc_connected_cb() is called.
 *
 * @param _hdlc HDLC instance data allocated by hdlc_init()
 */
void hdlc_reset_cb(hdlc_data_t *_hdlc) {
    log_debug("hdlc reset\n");

    // Connect to current thread if not already connected
    JNIEnv *env;
    attatch_current_thread(&env);

    // Building raw data array copy and call static method in Java
    (*env)->CallStaticVoidMethod(env, g_ctx.jniHdlcClz, g_ctx.hdlc_reset_cb_id);
}

/**
 * This is called when HDLC is connected, i.e. when the HDLC SABM frame is
 * acknowledged by peer. All frames sent between system startup OR
 * hdlc_reset_cb() and this are discarded.
 *
 * @param hdlc HDLC instance data allocated by hdlc_init()
 */
void hdlc_connected_cb(hdlc_data_t *hdlc) {
    log_debug("hdlc connected\n");

    // Connect to current thread if not already connected
    JNIEnv *env;
    attatch_current_thread(&env);

    // Building raw data array copy and call static method in Java
    (*env)->CallStaticVoidMethod(env, g_ctx.jniHdlcClz, g_ctx.hdlc_connected_cb_id);
}


/**
 * Called by hdlc to transmit raw serial data.
 *
 * This function should be implemented by the integration. `buf` pointer is
 * valid until function returns. The function is expected to send the entire
 * `buf` or nothing, hdlc does not explicitly handle partial writes, but the
 * underlying error handling will recover from it.
 *
 * Beware, that if this blocks, that will also cause blocking of receive
 * handling.
 *
 * @param hdlc HDLC instance data allocated by hdlc_init()
 * @param buf data
 * @param count length of data
 * @return < 0 in case of error, otherwise number of bytes sent (== count).
 */
int hdlc_os_tx(hdlc_data_t *_hdlc, const uint8_t *buf, uint32_t count) {
    // Connect to current thread if not already connected
    JNIEnv *env;
    attatch_current_thread(&env);

    // Building raw data array copy and call static method in Java
    jbyteArray rawDataCopy = (*env)->NewByteArray(env, count);
    (*env)->SetByteArrayRegion(env, rawDataCopy, 0, count, (const jbyte *) buf);
    (*env)->CallStaticVoidMethod(env, g_ctx.jniHdlcClz, g_ctx.hdlc_os_tx_id, rawDataCopy);
    (*env)->DeleteLocalRef(env, rawDataCopy);
    return count;
}

/**
 * Java calls into this function to initialize HDLC
 *
 * @param env JNIEnv pointer
 * @param thiz Java object
 * @return JNIEXPORT
 */
void initialize_hdlc_wrapper(JNIEnv *env, jobject thiz) {
    log_info("Initialize HDLC");
    hdlc_java_init(); // calls hdlc_init()
}

/**
 * Data has been received by serial device. Data is passed to HDLC from Java.
 *
 * @param env JNIEnv pointer
 * @param thiz Java object
 * @param buffer byte array received from serial
 * @param count number of bytes in buffer
 */
void hdlc_os_rx_wrapper(JNIEnv *env, jobject thiz, jbyteArray buffer, jint count) {
    jbyte *buf = (*env)->GetByteArrayElements(env, buffer, NULL);
    hdlc_os_rx(hdlc, (uint8_t *) buf, count);
    (*env)->ReleaseByteArrayElements(env, buffer, buf, 0);
}

/**
 * Java is sending a frame of data through HDLC.
 *
 * Once the data has been handed over to HDCL it will be processed and eventually
 * callbacks to hdlc_os_tx() will pass the raw data to the serial line.
 *
 * @param env JNIEnv pointer
 * @param thiz Java object
 * @param frame byte array to be sent using HDLC
 * @param len number of bytes in frame
 */
void hdlc_send_frame_wrapper(JNIEnv *env, jobject thiz, jbyteArray frame, jint len) {
    jbyte *buf = (*env)->GetByteArrayElements(env, frame, NULL);
    log_debug("Sending frame data %d bytes", len);
    // Call to Get<*>ArrayElements must always be matched with a Release<*>ArrayElements. This
    // will free the data to java garbage collector and also free any allocated data used in
    // Get<*>ArrayElements.
    // Create a local copy of data to be transmitted which will be freed in hdlc_frame_sent_cb()
    // Alternatively the reference to jbyteArray could be stored and used in hdlc_frame_sent_cb()
    // to free the data to java garbage collector using ReleaseByteArrayElements()
    uint8_t *msg = malloc(len);
    memcpy(msg, buf, len);
    hdlc_send_frame(hdlc, msg, len);
    (*env)->ReleaseByteArrayElements(env, frame, buf, 0);
}
