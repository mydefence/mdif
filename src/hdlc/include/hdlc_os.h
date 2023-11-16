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
 * Program/file : hdlc_os.h                                                    *
 *                                                                             *
 * Description  : OS Abstraction layer. These function are used to integrate   *
 *              : hdlc ito your OS                                             *
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
#ifndef _HDLC_OS_H_
#define _HDLC_OS_H_
#include <inttypes.h>
#include <stdio.h>

// The include path must contain this port specific header file.
#include <hdlc_port.h>

/// HDLC instance data, allocated by hdlc_init(). The actual struct allocated by
/// HDLC is larger than this and contains internal data.
typedef struct {
    /// The user data field is a pointer the application may use to refer to any
    /// data it needs. The integration may set it to anything it wishes
    /// including NULL. It will not be touched by hdlc.
    void *user_data;

    /// Number of frames in the TX queue. Can be used to throttle the sender.
    /// Alternatively a sender may use hdlc_frame_sent_cb() for throttling.
    unsigned int hdlc_tx_queue_size;
} hdlc_data_t;

/// Called by integration to initialize an hdlc instance. May be called multiple
/// times if more than one HDLC device is connected.
///
/// @param user_data pointer to user data. The pointer will be copied to
/// hdlc_data_t.
hdlc_data_t *hdlc_init(void *user_data);

/// Called by integration to free resources allocated by hdlc_init(). The
/// integration is responsible for cleanup of anything in hdlc->user_data. After
/// call to this, no other `hdlc_*()`/`hdlc_os_*()` functions may be called and
/// `hdlc` must not be accessed. Note: This function uses various `hdlc_os_*()`
/// callbacks, so do not free any instance resources until AFTER this call.
void hdlc_free(hdlc_data_t *hdlc);

/// Called by hdlc to transmit raw serial data.
///
/// This function should be implemented by the integration. `buf` pointer is
/// valid until function returns. The function is expected to send the entire
/// `buf` or nothing, hdlc does not explicitly handle partial writes, but the
/// underlying error handling will recover from it.
///
/// Beware, that if this blocks, that will also cause blocking of receive
/// handling.
///
/// @param buf data
/// @param count length of data
/// @return < 0 in case of error, otherwise number of bytes sent (== count).
int hdlc_os_tx(hdlc_data_t *hdlc, const uint8_t *buf, uint32_t count);

/// Called by integration when new raw serial data is received.
///
/// This function called from the integration. data pointed to by `buf` may be
/// freed or overwritten after the function call returns.
///
/// @param buf data @param count length of data
void hdlc_os_rx(hdlc_data_t *hdlc, const uint8_t *buf, uint32_t count);

/// Called by hdlc to (re)start retransmission timer.
///
/// Integration must provide a single-shot timer for retransmission handling.
/// This function is called after transmission of the first outstanding packet.
/// Integration must select an appropriate timeout value for the time it is
/// expected to receive an ACK, i.e. the round trip time of the largest frame
/// plus ack.
///
/// This function will also be called to restart timer when an ACK is received,
/// and there are still outstanding frames.
void hdlc_os_start_timer(hdlc_data_t *hdlc);

/// Called by hdlc to stop retransmission timer started by
/// hdlc_os_start_timer().
void hdlc_os_stop_timer(hdlc_data_t *hdlc);

/// Called by integration when retransmission timer expires.
///
/// Note, that the function will call hdlc_os_exit_critical_section(),
/// hdlc_os_exit_critical_section(), hdlc_os_start_timer() and hdlc_os_tx().
void hdlc_os_timeout(hdlc_data_t *hdlc);

/// Called by integration to indicate link loss. This will reset HDLC.
/// hdlc_reset_cb() and later hdlc_connected_cb() will be called. The
/// integration may omit this.
void hdlc_os_link_lost(hdlc_data_t *hdlc);

/// Called by hdlc lock a mutex. If integration does not use multi-threading it
/// may provide empty functions.
void hdlc_os_enter_critical_section(hdlc_data_t *hdlc);

/// Called by hdlc unlock a mutex. If integration does not use multi-threading
/// it may provide empty functions
void hdlc_os_exit_critical_section(hdlc_data_t *hdlc);

#ifndef HDLC_RETRANSMIT_CNT
/// Number of retransmissions to attempt before resetting the link.
///
/// Thus the timeout is this value times the hdlc_os_start_timer() time.
#define HDLC_RETRANSMIT_CNT 20
#endif

#ifndef HDLC_KEEP_ALIVE_CNT
/// Number of timeouts with no data transmission before sending keep-alive
/// packet.
///
/// Thus detection of broken link takes `(HDLC_KEEP_ALIVE_CNT +
/// HDLC_RETRANSMIT_CNT)` * hdlc_os_start_timer() time
#define HDLC_KEEP_ALIVE_CNT 30
#endif

#ifndef log_trace
/// Define log_ macros to enable logging. The integration may omit this. Default
/// no logging is enabled.
#define log_trace(...)
#define log_debug(...)
#define log_info(...)
#define log_warn(...)
#define log_error(...)
#define log_fatal(...)
#endif

#endif // _HDLC_OS_H_