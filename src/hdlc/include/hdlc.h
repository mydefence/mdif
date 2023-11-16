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
 * Program/file : hdlc.h                                                       *
 *                                                                             *
 * Description  : Types and prototypes for HDLC                                *
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
#ifndef _HDLC_H_
#define _HDLC_H_

#include "hdlc_os.h" // hdlc_data_t
#include <inttypes.h>
#include <stddef.h>

/// Maximum length of HDLC data frame.
#define HDLC_MAX_FRAME_LEN 2000

typedef enum {
    HDLC_SUCCESS = 0,
    /// Call to hdlc_os_*() function in OS Abstraction Layer failed. On some OS
    /// more information may be available, e.g. in errno.
    HDLC_OS_ERROR = -1,
    /// HDLC layer is not connected, i.e. wait for hdlc_connected_cb().
    HDLC_NOT_CONNECTED = -2,
    /// Frame is too long
    HDLC_FRAME_TOO_LONG = -3,
} hdlc_result_t;

struct hdlc_stat_t {
    /// Data frames received (not including retransmissions)
    uint32_t rx;
    /// UI frames received (not including retransmissions)
    uint32_t ui_rx;
    /// Out-of-sequence rx frames
    uint32_t rx_retrans;
    /// Failure to decode HDLC frame
    uint32_t rx_err;
    /// Ack frames received
    uint32_t rx_ack;
    /// Nack frames received
    uint32_t rx_nack;
    /// Data frames transmitted (not including retransmissions)
    uint32_t tx;
    /// UI frames transmitted (not including retransmissions)
    uint32_t ui_tx;
    /// Failure of hdlc_os_tx()
    uint32_t tx_err;
    /// Data frames retransmitted (including retransmission frames)
    uint32_t tx_retrans;
    /// Ack frames transmitted
    uint32_t tx_ack;
    /// Nack frames transmitted
    uint32_t tx_nack;
    /// Number of keep alive frames transmitted (not including retransmissions)
    uint32_t tx_keep_alive;
    /// Number of times sequence number handling has been reset
    uint32_t reset;
};

/// Various statistics about the HDLC layer, that may be useful for diagnosing
/// link performance.
extern struct hdlc_stat_t hdlc_stat;

/// Reliable transmission of one data frame
///
/// This function is used to send one data frame. The frame is queued for
/// transmission and the function returns immediately. When the frame has been
/// successfully sent, the callback function hdlc_frame_sent_cb() is called with
/// the same parameters. If frame transmission times out, hdlc_reset_cb() is
/// called followed by hdlc_frame_sent_cb().
///
/// @param h HDLC instance data allocated by hdlc_init()
/// @param frame Pointer to data to send. The pointer must be valid until
/// hdlc_frame_sent_cb() is called
/// @param len Length of data.
/// @return 0 in case of success. Success means the data was correctly queued
/// for transmission, not that it has been sent. See \ref hdlc_result_t for error
/// codes. In case of error, hdlc_frame_sent_cb() is not called.
hdlc_result_t hdlc_send_frame(hdlc_data_t *h, const uint8_t *frame, uint32_t len);

/// Callback function called when a frame has been sent, or otherwise discarded.
/// @param h HDLC instance data allocated by hdlc_init()
/// @param frame parameter from hdlc_send_frame()
/// @param len parameter from hdlc_send_frame()
void hdlc_frame_sent_cb(hdlc_data_t *h, const uint8_t *frame, uint32_t len);

/// Unreliable transmission of one data frame
///
/// This function is used to send one HDLC UI frame. The frame is attempted
/// transmitted immediately. There is no way of knowing whether the frame has
/// been delivered. frame may be freed immediately after return.
/// hdlc_frame_sent_cb() is not called.
///
/// This frame may be dropped, delivered before, or delivered after
/// unacknowledged frames already queued with hdlc_send_frame(). Data queued
/// with hdlc_send_frame() or hdlc_send_frame_unacknowledged() after this frame,
/// will always be delivered after this frame was delivered (if it is
/// delivered).
///
/// @param h HDLC instance data allocated by hdlc_init()
/// @param frame Pointer to data to send
/// @param len Length of data.
/// @return 0 in case of success. Success means the data was correctly queued
/// for transmission, not that it has been sent. See \ref hdlc_result_t for error
/// codes.
hdlc_result_t hdlc_send_frame_unacknowledged(hdlc_data_t *h, const uint8_t *frame, uint32_t len);

/// Callback function called when a frame (data or UI) has been received.
///
/// This function is called when a frame has been received. Frames are always
/// delivered in the same order they are sent, with the exception that UI frames
/// may overtake data frames.
///
/// @param h HDLC instance data allocated by hdlc_init()
/// @param frame Pointer to a received HDLC frame. The pointer is invalid
/// after this function returns.
/// @param len Length of the frame
void hdlc_recv_frame_cb(hdlc_data_t *h, uint8_t *frame, uint32_t len);

/// This is called when HDLC sequence number are reset, which happens either
/// 1. In case of retransmission timeout (see HDLC_RETRANSMIT_CNT in hdlc_os.h)
/// 2. If sequence numbers are out of sync
/// 3. If HDLC SABM frame is received from peer
///
/// This is followed by cleanup of the current TX queue including calling
/// hdlc_frame_sent_cb(). hdlc_send_frame() and hdcl_send_frame_unacknowledged()
/// will fail until hdlc_connected_cb() is called.
///
/// @param h HDLC instance data allocated by hdlc_init()
void hdlc_reset_cb(hdlc_data_t *h);

/// This is called when HDLC is connected, i.e. when the HDLC SABM frame is
/// acknowledged by peer. All frames sent between system startup OR
/// hdlc_reset_cb() and this are discarded.
///
/// @param h HDLC instance data allocated by hdlc_init()
void hdlc_connected_cb(hdlc_data_t *h);

#endif // _HDLC_H_