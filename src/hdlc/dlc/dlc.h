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
 * Program/file : dlc.h                                                        *
 *                                                                             *
 * Description  : Header file with type definitions used by the dlc layer      *
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
#ifndef _DLC_H_
#define _DLC_H_

#include "queue.h"
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include "../dlc/dlc.h"
#include "../include/hdlc.h"
#include "../yahdlc/yahdlc.h"

#ifdef MDIF_FRAGMENT_SUPPORT
#include "../fragmentation/fragmentation.h"
#endif

struct txq_entry {
    // -1 if not transmitted yet, otherwise 0-7
    int seq_no;
    // Note frame may be null, indicating empty (keep-alive) frame
    const uint8_t *frame;
    uint32_t len;
    // clang-format off
    TAILQ_ENTRY(txq_entry) q;
    // clang-format on
};
TAILQ_HEAD(txq_tailhead, txq_entry);

enum dlc_state {
    // Sending SABM, waiting for UA from peer.
    RST_REQUIRED,
    // RST_COMPLETE_WAIT is a hack to handle the case where the peer sends SABM
    // and while mutex is locked during handling of SABM our TX thread creates a
    // frame and ends up waiting for the mutex to send it. After processing SABM
    // we call hdlc_reset() to inform app that we have a new peer. But the TX
    // thread is in the process of sending an old frame. This timeout allows (no
    // guarantee though!) the TX thread to get mutex and attempt to send the
    // frame, before we go to RST_COMPLETE or ACTIVE, which means
    // hdlc_send_frame() will fail with HDLC_NOT_CONNECTED and the old frame is
    // thrown away.
    //
    // This implies there will be a short period of 1 timeout after SABM from
    // peer, where we ignore frames from peer. It will have to retransmit.
    RST_COMPLETE_WAIT,
    // Peer has been reset, no data yet. Difference between this and ACTIVE is
    // whether we need to reset state on SABM.
    RST_COMPLETE,
    // Data transfer active
    ACTIVE
};

typedef struct {
    // External user interface, must be first
    hdlc_data_t ext;
    yahdlc_state_t yahdlc;
    // DLC state.
    struct {
        // N(R): Next expected rx seq no. 0-7
        uint8_t expected_rx_seq_no;
        // N(S): Next seq no to use for tx. 0-7
        uint8_t tx_seq_no;

        // Number of frames sent, waiting for ack. 0-MAX_OUTSTANDING_FRAMES
        unsigned int tx_outstanding;
        // Number of retransmission attempts of first frame in txq
        int retransmit_attempts;
        // Flag indicating need for retranmission
        int retransmit_on_ack;
        // Flag indicating we have received un-acked data
        int ack_pending;
        // Number of timeouts with no data transmission
        int keep_alive_counter;

        struct txq_tailhead txq;
        struct txq_tailhead freeq;
        struct txq_entry *last_tx;

        enum dlc_state state;
    } dlc;
#ifdef MDIF_FRAGMENT_SUPPORT
    struct frag_data_t frag_data;
#endif
    // +2 needed because yahdlc writes checksum bytes to hdlc_frame (not returned)
    uint8_t hdlc_frame[HDLC_MAX_FRAME_LEN + 2];

} hdlc_intdata_t;

// We may cast directly between hdlc_data_t and hdlc_intdata_t
static_assert(offsetof(hdlc_intdata_t, ext) == 0, "ext must be first member");

#endif // _DLC_H_