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
 * Program/file : dlc.c                                                        *
 *                                                                             *
 * Description  : Implementation of the dlc layer                              *
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
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/hdlc.h"
#include "../include/hdlc_os.h"
#include "../yahdlc/yahdlc.h"
#include "dlc.h"
#include "queue.h"

#ifdef MDIF_FRAGMENT_SUPPORT
#define hdlc_send_frame hdlc_dlc_send_frame
#define hdlc_frame_sent_cb hdlc_dlc_sent_cb
#define hdlc_recv_frame_cb hdlc_dlc_recv_frame_cb
#define hdlc_reset_cb hdlc_dlc_reset_cb
#define hdlc_init hdlc_dlc_init
#define hdlc_free hdlc_dlc_free
#endif

struct hdlc_stat_t hdlc_stat;

static_assert(YAHDLC_MAX_FRAME_LEN == HDLC_MAX_FRAME_LEN, "Max frame len mismatch");

// Must be in range 1-7
#ifndef MAX_OUTSTANDING_FRAMES
#define MAX_OUTSTANDING_FRAMES 2
#endif

static void send_sabm_frame(hdlc_intdata_t *hu);
static void reset(hdlc_intdata_t *hi, hdlc_reset_cause_t cause);

static void hdlc_reset(hdlc_intdata_t *hi)
{
    memset(&hi->dlc, 0, sizeof(hi->dlc));
    hi->ext.hdlc_tx_queue_size = 0;
    TAILQ_INIT(&hi->dlc.txq);
    TAILQ_INIT(&hi->dlc.freeq);
    hi->dlc.state = RST_REQUIRED;
    hdlc_os_start_timer(&hi->ext);
    yahdlc_get_data_reset_with_state(&hi->yahdlc);
}

hdlc_data_t *hdlc_init(void *user_data)
{
    hdlc_intdata_t *hi = HDLC_OS_MALLOC(sizeof(hdlc_intdata_t));
    hi->ext.user_data = user_data;
    hdlc_os_enter_critical_section(&hi->ext);
    hdlc_reset(hi);
    send_sabm_frame(hi);
    hdlc_os_exit_critical_section(&hi->ext);
    return &hi->ext;
}

void hdlc_free(hdlc_data_t *h)
{
    hdlc_intdata_t *hi = (hdlc_intdata_t *)h;
    hdlc_os_enter_critical_section(&hi->ext);
    reset(hi, HDLC_RESET_CAUSE_APPLICATION_FREE);
    // releases mutex
    hdlc_os_stop_timer(h);
    HDLC_OS_FREE(hi);
}

#ifdef _MSC_BUILD
#pragma warning(push)
#pragma warning(disable : 4100)
#endif

static void dbg_dump(const char *info, const uint8_t *buf, uint32_t count)
{
#if 0
    log_debug("%s %d bytes: ", info, count);
    for (int i = 0; i < count; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
#endif
}

static void dbg_validate_state(hdlc_intdata_t *hi, const char *info)
{
#ifdef STRESS_TEST
    char str[1024], *s = str;
    // dump txq
    struct txq_entry *txe;
    int txq_cnt = 0, outstanding_cnt = 0;
    TAILQ_FOREACH(txe, &hi->dlc.txq, q)
    {
        txq_cnt++;
        s += sprintf(s, " [%d:%d]", txe->seq_no, txe->len);
        if (hi->dlc.last_tx == txe) {
            s += sprintf(s, "*");
            outstanding_cnt = txq_cnt;
        }
    }
    s += sprintf(s, " (entries %d, out %d)\n", txq_cnt, outstanding_cnt);
    log_debug("state [%s] hi->dlc.tx_outstanding=%d hdlc_tx_queue_size=%d. %s", info, hi->dlc.tx_outstanding, hi->ext.hdlc_tx_queue_size, s);
    assert(hi->dlc.tx_outstanding <= hi->ext.hdlc_tx_queue_size);
    assert(hi->dlc.tx_outstanding <= MAX_OUTSTANDING_FRAMES);
    assert(txq_cnt == hi->ext.hdlc_tx_queue_size);
    assert(outstanding_cnt == hi->dlc.tx_outstanding);
#endif
}

#ifdef _MSC_BUILD
#pragma warning(pop)
#endif

static void tx_data_frame(hdlc_intdata_t *hi, struct txq_entry *txe)
{
    uint8_t hdlcbuf[YAHDLC_MAX_ENCODED_LEN];
    unsigned int hdlclen;
    yahdlc_control_t ctrl_tx = {
        .frame = YAHDLC_FRAME_DATA,
        .recv_seq_no = hi->dlc.expected_rx_seq_no,
    };

    if (txe->seq_no == -1) {
        hdlc_stat.tx++;
        txe->seq_no = ctrl_tx.send_seq_no = hi->dlc.tx_seq_no;
        hi->dlc.tx_seq_no = (hi->dlc.tx_seq_no + 1) & 7;
        hi->dlc.retransmit_attempts = 0;
    } else {
        hdlc_stat.tx_retrans++;
        ctrl_tx.send_seq_no = (uint8_t)txe->seq_no;
        log_info("retransmission of seq_no=%d", ctrl_tx.send_seq_no);
    }

    int res = yahdlc_frame_data(&ctrl_tx, (const char *)txe->frame, txe->len, (char *)hdlcbuf, &hdlclen);
    if (res != 0) {
        log_fatal("ERROR yahdlc_frame_data res=%d", res);
        exit(1);
    }
    hi->dlc.ack_pending = 0;
    log_info("tx frame seq=%d ack=%d framelen=%d enclen=%d data[0]=%2.2x", ctrl_tx.send_seq_no, ctrl_tx.recv_seq_no, txe->len, hdlclen, txe->frame ? txe->frame[0] : -1);
    dbg_dump("framed data", hdlcbuf, hdlclen);

    res = hdlc_os_tx(&hi->ext, hdlcbuf, hdlclen);
    if (res != (int)hdlclen) {
        // handled by normal retransmission timeout
        log_warn("hdlc_os_tx res=%d, expected=%d", res, hdlclen);
    }

    if (hi->dlc.last_tx == NULL || TAILQ_NEXT(hi->dlc.last_tx, q) == txe) {
        hi->dlc.last_tx = txe;
    }
}

static void hdlc_insert_frame(hdlc_intdata_t *hi, struct txq_entry *txe)
{
    TAILQ_INSERT_TAIL(&hi->dlc.txq, txe, q);
    hi->ext.hdlc_tx_queue_size++;
    if (!hi->dlc.retransmit_on_ack && hi->dlc.tx_outstanding < MAX_OUTSTANDING_FRAMES) {
        hi->dlc.tx_outstanding++;
        tx_data_frame(hi, txe);
        if (hi->dlc.tx_outstanding == 1) {
            hdlc_os_start_timer(&hi->ext);
        }
    }
    dbg_validate_state(hi, __FUNCTION__);
}

hdlc_result_t hdlc_send_frame(hdlc_data_t *h, const uint8_t *frame, uint32_t len)
{
    hdlc_intdata_t *hi = (hdlc_intdata_t *)h;
    log_info("hdlc_send_frame len=%d", len);
    if (len > HDLC_MAX_FRAME_LEN) {
        log_error("HDLC frame length %d too long", len);
        return HDLC_FRAME_TOO_LONG;
    }
    dbg_dump("hdlc_send_frame", frame, len);
    struct txq_entry *txe = HDLC_OS_MALLOC(sizeof(struct txq_entry));
    txe->frame = frame;
    txe->len = len;
    txe->seq_no = -1;

    hdlc_os_enter_critical_section(&hi->ext);
    if (hi->dlc.state < RST_COMPLETE) {
        log_warn("hdlc_send_frame NOT_CONNECTED");
        hdlc_os_exit_critical_section(&hi->ext);
        HDLC_OS_FREE(txe);
        return HDLC_NOT_CONNECTED;
    }
    hi->dlc.state = ACTIVE;

    hdlc_insert_frame(hi, txe);
    hdlc_os_exit_critical_section(&hi->ext);

    return HDLC_SUCCESS;
}

// For frames without data. recv_seq_no is ignored for some frame types.
static void send_ctrl_frame(hdlc_intdata_t *hi, yahdlc_frame_t frame)
{
    yahdlc_control_t ctrl_ack = {.frame = frame, .recv_seq_no = hi->dlc.expected_rx_seq_no};
    uint8_t hdlcbuf[YAHDLC_MAX_ENCODED_LEN];
    unsigned int hdlclen;
    int res = yahdlc_frame_data(&ctrl_ack, NULL, 0, (char *)hdlcbuf, &hdlclen);
    if (res != 0) {
        log_fatal("ERROR yahdlc_frame_data res=%d", res);
        exit(1);
    }

    res = hdlc_os_tx(&hi->ext, hdlcbuf, hdlclen);
    if (res != (int)hdlclen) {
        hdlc_stat.tx_err++;
        log_warn("hdlc_os_tx res=%d, len=%d", res, hdlclen);
    }
}

static void send_ack_frame(hdlc_intdata_t *hi)
{
    hdlc_stat.tx_ack++;
    send_ctrl_frame(hi, YAHDLC_FRAME_ACK);
    hi->dlc.ack_pending = 0;
}

static void send_nack_frame(hdlc_intdata_t *hi)
{
    log_info("send NACK %d", hi->dlc.expected_rx_seq_no);
    hdlc_stat.tx_nack++;
    send_ctrl_frame(hi, YAHDLC_FRAME_NACK);
    hi->dlc.ack_pending = 0;
}

static void send_sabm_frame(hdlc_intdata_t *hi)
{
    log_info("send SABM (reset)");
    send_ctrl_frame(hi, YAHDLC_FRAME_SABM);
    hdlc_os_start_timer(&hi->ext);
}

static void send_ua_frame(hdlc_intdata_t *hi)
{
    log_info("send UA");
    send_ctrl_frame(hi, YAHDLC_FRAME_UA);
}

// UI frames are transmitted and immediately discarded. There is no ack. We
// ignore any errors in tx.
hdlc_result_t hdlc_send_frame_unacknowledged(hdlc_data_t *h, const uint8_t *frame, uint32_t len)
{
    hdlc_intdata_t *hi = (hdlc_intdata_t *)h;

    log_info("hdlc_send_frame len=%d", len);
    dbg_dump("hdlc_send_frame", frame, len);

    uint8_t hdlcbuf[YAHDLC_MAX_ENCODED_LEN];
    unsigned int hdlclen;
    yahdlc_control_t ctrl_tx = {.frame = YAHDLC_FRAME_UI};

    int res = yahdlc_frame_data(&ctrl_tx, (const char *)frame, len, (char *)hdlcbuf, &hdlclen);
    if (res != 0) {
        log_fatal("ERROR yahdlc_frame_data res=%d", res);
        exit(1);
    }

    log_info("hdlc_send_frame_unacknowledged UI frame framelen=%d enclen=%d data[0]=%2.2x", len, hdlclen, frame[0]);
    dbg_dump("hdlc_send_frame_unacknowledged", frame, len);
    dbg_dump("framed data", hdlcbuf, hdlclen);

    hdlc_os_enter_critical_section(&hi->ext);
    if (hi->dlc.state < RST_COMPLETE) {
        log_warn("hdlc_send_frame_unacknowledged NOT_CONNECTED");
        hdlc_os_exit_critical_section(&hi->ext);
        return HDLC_NOT_CONNECTED;
    }
    hdlc_stat.ui_tx++;
    res = hdlc_os_tx(&hi->ext, hdlcbuf, hdlclen);
    hdlc_os_exit_critical_section(&hi->ext);
    if (res != (int)hdlclen) {
        // Errors ignored
        log_warn("hdlc_send_frame_unacknowledged res=%d, expected=%d", res, hdlclen);
        return res == -1 ? -1 : -2;
    }
    return 0;
}

// Handle received ack for our send data. Must be followed by call to
// rx_ack_cleanup() after unlocking mutex!
static void rx_ack(hdlc_intdata_t *hi, uint8_t ack_seq_no)
{
    assert(ack_seq_no < 8);
    struct txq_entry *txq_head = TAILQ_FIRST(&hi->dlc.txq);

    if (txq_head == NULL) {
        log_info("rx_ack %d but txq is empty", ack_seq_no);
        return;
    }
    // ack_seq_no is next expected data sequence number, i.e. ack of the
    // previous seq no. So if it is identical to the first on our tx queue, it
    // was an ack for the previously sent and previously acked data.
    if (ack_seq_no == txq_head->seq_no) {
        log_info("rx_ack %d outdated", ack_seq_no);
        return;
    }

    log_info("rx_ack %d, head seq %d, queue length %d", ack_seq_no, txq_head->seq_no, hi->ext.hdlc_tx_queue_size);

    // acked fragments are moved from tx queue to temporary list, so we can free
    // and callback with mutex unlocked.
    while (txq_head) {
        log_debug("txq remove %d", txq_head->seq_no);
        struct txq_entry *next = TAILQ_NEXT(txq_head, q);
        TAILQ_REMOVE(&hi->dlc.txq, txq_head, q);
        TAILQ_INSERT_TAIL(&hi->dlc.freeq, txq_head, q);

        if (hi->dlc.last_tx == txq_head) {
            hi->dlc.last_tx = NULL;
        }
        txq_head = next;
        assert(hi->ext.hdlc_tx_queue_size);
        hi->ext.hdlc_tx_queue_size--;
        assert(hi->dlc.tx_outstanding);
        hi->dlc.tx_outstanding--;
        if (!txq_head || txq_head->seq_no == ack_seq_no || txq_head->seq_no == -1) {
            break;
        }
    }

    // Even though this frame may have been retransmitted a number of times, we
    // reset counter, because we just learned that link is working.
    hi->dlc.retransmit_attempts = 0;

    if (hi->dlc.retransmit_on_ack) {
        struct txq_entry *txe;
        TAILQ_FOREACH(txe, &hi->dlc.txq, q)
        {
            if (txe->seq_no == -1) {
                break;
            }
            log_info("retransmit (on ack) %d", txe->seq_no);
            tx_data_frame(hi, txe);
        }
        hi->dlc.retransmit_on_ack = 0;
    }

    while (hi->dlc.tx_outstanding < MAX_OUTSTANDING_FRAMES && hi->ext.hdlc_tx_queue_size > hi->dlc.tx_outstanding) {
        if (!hi->dlc.last_tx) {
            assert(hi->dlc.tx_outstanding == 0);
            hi->dlc.last_tx = TAILQ_FIRST(&hi->dlc.txq);
        } else {
            hi->dlc.last_tx = TAILQ_NEXT(hi->dlc.last_tx, q);
        }

        hi->dlc.tx_outstanding++;
        tx_data_frame(hi, hi->dlc.last_tx);
    }

    assert(hi->dlc.tx_outstanding || hi->dlc.last_tx == NULL);

    hdlc_os_start_timer(&hi->ext);
    if (!hi->dlc.tx_outstanding) {
        if (hi->dlc.ack_pending) {
            log_info("send pending ACK %d", hi->dlc.expected_rx_seq_no);
            send_ack_frame(hi);
        }
    }
}

// Must be called after rx_ack() and after releasing mutex, in order to free
// allocated memory.
static void rx_ack_cleanup(hdlc_intdata_t *hi)
{
    struct txq_entry *fe = TAILQ_FIRST(&hi->dlc.freeq);
    while (fe) {
        if (fe->frame) {
            hdlc_frame_sent_cb(&hi->ext, fe->frame, fe->len);
        }
        struct txq_entry *next = TAILQ_NEXT(fe, q);
        HDLC_OS_FREE(fe);
        fe = next;
    }
    TAILQ_INIT(&hi->dlc.freeq);
}

// Called after receiving a valid in-sequence I-frame (data). Trigger ack
// response unless we expect we can piggyback it.
static void ack_recv_data(hdlc_intdata_t *hi, uint8_t rx_seq_no)
{
    hi->dlc.expected_rx_seq_no = (rx_seq_no + 1) & 7;
    if (hi->ext.hdlc_tx_queue_size && hi->dlc.tx_outstanding < MAX_OUTSTANDING_FRAMES) {
        // ack will be sent on next tx transmission. There may not be any
        // more tx transmissions, in which case we send ack when tx queue
        // goes empty. (If hi->dlc.tx_outstanding was max'ed we could risk a
        // dead-lock with filled buffers in both ends.)
        log_info("delay ack %d", rx_seq_no);
        hi->dlc.ack_pending = 1;
        return;
    }

    log_info("send immediate ACK %d", hi->dlc.expected_rx_seq_no);
    send_ack_frame(hi);
}

// Trigger retransmission of all unacked frames due to timeout.
//
// Retrans timeout is expected to be little more than the RTT, so if we send all
// queued frames, we risk sending more data than possible on the wire. So we
// only send one frame, and set a flag to send all when we get ack (at which
// point our OS's send queue should be empty).
void hdlc_os_timeout(hdlc_data_t *h)
{
    hdlc_intdata_t *hi = (hdlc_intdata_t *)h;
    log_info("hdlc_os_timeout");
    hdlc_os_enter_critical_section(&hi->ext);
    if (hi->dlc.state == RST_REQUIRED) {
        send_sabm_frame(hi);
        hdlc_os_exit_critical_section(&hi->ext);
        return;
    }

    if (hi->dlc.state == RST_COMPLETE_WAIT) {
        hi->dlc.state = RST_COMPLETE;
        hdlc_os_start_timer(&hi->ext);
        hdlc_os_exit_critical_section(&hi->ext);
        hdlc_connected_cb(&hi->ext);
        return;
    }

    if (hi->ext.hdlc_tx_queue_size && hi->dlc.tx_outstanding) {
        struct txq_entry *txe = TAILQ_FIRST(&hi->dlc.txq);
        log_info("retransmit %d (attempt:%d)", txe->seq_no, hi->dlc.retransmit_attempts);
        if (++hi->dlc.retransmit_attempts == HDLC_RETRANSMIT_CNT) {
            reset(hi, hi->dlc.keep_alive_counter >= HDLC_KEEP_ALIVE_CNT ? HDLC_RESET_CAUSE_TIMEOUT_KEEP_ALIVE : HDLC_RESET_CAUSE_TIMEOUT_RETRANSMIT);
            // reset will drop the mutex
            return;
        }
        tx_data_frame(hi, txe);
        hi->dlc.retransmit_on_ack = 1;
    } else {
        hi->dlc.keep_alive_counter++;
        if (hi->dlc.keep_alive_counter == HDLC_KEEP_ALIVE_CNT) {
            log_info("send keep-alive");
            struct txq_entry *txe = HDLC_OS_MALLOC(sizeof(struct txq_entry));
            txe->frame = NULL;
            txe->len = 0;
            txe->seq_no = -1;
            hdlc_insert_frame(hi, txe);
            hdlc_stat.tx_keep_alive++;
        }
    }
    hdlc_os_start_timer(&hi->ext);
    hdlc_os_exit_critical_section(&hi->ext);
}

void hdlc_os_rx(hdlc_data_t *h, const uint8_t *buf, uint32_t count)
{
    hdlc_intdata_t *hi = (hdlc_intdata_t *)h;

    unsigned int hdlc_frame_len;

    log_info("hdlc_os_rx %d bytes", count);
    dbg_dump("hdlc_os_rx", buf, count);

    int need_nack = 0;
    yahdlc_frame_t prev_frame = YAHDLC_FRAME_NOT_SUPPORTED;
    do {
        assert(count > 0);
        int res = yahdlc_get_data_with_state(&hi->yahdlc, (char *)buf, count, (char *)hi->hdlc_frame, &hdlc_frame_len);
        if (res == -ENOMSG) {
            // not enough data for a frame. yahdlc has stored whatever it could
            // in hdlc_frame. The closing flag is never removed.
            if (count > 1 || buf[count - 1] == YAHDLC_FLAG_SEQUENCE) {
                log_info("hdlc_os_rx. not enough data for a frame. Remaining %d", count);
            }
            break;
        } else if (res == -EIO) {
            log_warn("hdlc_os_rx. Checksum error. Discard %d", hdlc_frame_len);
            hdlc_stat.rx_err++;
            buf += hdlc_frame_len;
            assert(count >= hdlc_frame_len);
            count -= hdlc_frame_len;
            continue;
        } else if (res < 0) {
            log_fatal("ERROR yahdlc_get_data returned %d", res);
            exit(1);
        }

        dbg_dump("decoded", hi->hdlc_frame, hdlc_frame_len);
        hi->dlc.keep_alive_counter = 0;

        switch (hi->yahdlc.control.frame) {
        case YAHDLC_FRAME_UI:
            log_info("hdlc_os_rx. Got ui framelen=%d using %d bytes raw data. data[0]=%2.2x",
                     hdlc_frame_len,
                     res,
                     hi->hdlc_frame[0]);
            break;
        case YAHDLC_FRAME_DATA:
            if (hdlc_frame_len == 0) {
                log_info("hdlc_os_rx. Got keep-alive seq=%d ack=%d using %d bytes raw data.",
                         hi->yahdlc.control.send_seq_no,
                         hi->yahdlc.control.recv_seq_no,
                         res);
            } else {
                log_info("hdlc_os_rx. Got data seq=%d ack=%d framelen=%d using %d bytes raw data. data[0]=%2.2x",
                         hi->yahdlc.control.send_seq_no,
                         hi->yahdlc.control.recv_seq_no,
                         hdlc_frame_len,
                         res,
                         hi->hdlc_frame[0]);
            }
            // Set keep alive counter to 1 for data frames to reduce likelyhood
            // of both sides sending keep-alive at the same time. (OK if they
            // do, but better to keep the line open for real data.)
            hi->dlc.keep_alive_counter = 1;
            break;
        case YAHDLC_FRAME_ACK:
        case YAHDLC_FRAME_NACK:
            log_info("hdlc_os_rx. Got %s ack=%d, using %d bytes raw data:",
                     hi->yahdlc.control.frame == YAHDLC_FRAME_ACK ? "ACK" : "NACK",
                     hi->yahdlc.control.recv_seq_no,
                     res);
            break;
        case YAHDLC_FRAME_SABM:
            if (prev_frame == YAHDLC_FRAME_SABM) {
                log_debug("hdlc_os_rx. Ignore duplicate SABM.");
                goto label_continue;
            }
            log_info("hdlc_os_rx. Got SABM. State:%i", hi->dlc.state);
            break;
        case YAHDLC_FRAME_UA:
            log_info("hdlc_os_rx. Got UA");
            break;
        case YAHDLC_FRAME_NOT_SUPPORTED:
            log_warn("hdlc_os_rx. Got unknown frame type %d", hi->yahdlc.control.frame);
            break;
        }
        prev_frame = hi->yahdlc.control.frame;

        hdlc_os_enter_critical_section(&hi->ext);

        if (hi->dlc.state < RST_COMPLETE && (hi->yahdlc.control.frame != YAHDLC_FRAME_SABM && hi->yahdlc.control.frame != YAHDLC_FRAME_UA)) {
            log_warn("hdlc_os_rx. Ignore frame due to RST_REQUIRED state");
            hdlc_os_exit_critical_section(&hi->ext);
            goto label_continue;
        }

        switch (hi->yahdlc.control.frame) {
        case YAHDLC_FRAME_DATA: {
            int in_order = hi->dlc.expected_rx_seq_no == hi->yahdlc.control.send_seq_no;
            rx_ack(hi, hi->yahdlc.control.recv_seq_no);
            if (in_order) {
                hdlc_stat.rx++;
                hi->dlc.state = ACTIVE;

                ack_recv_data(hi, hi->yahdlc.control.send_seq_no);
                need_nack = 0;
            } else {
                hdlc_stat.rx_retrans++;
                log_warn("hdlc_os_rx. Got out-of-order frame. Expected %d, got %d",
                         hi->dlc.expected_rx_seq_no, hi->yahdlc.control.send_seq_no);

                need_nack = 1;
            }
            dbg_validate_state(hi, __FUNCTION__);
            hdlc_os_exit_critical_section(&hi->ext);
            if (in_order && hdlc_frame_len) {
                hdlc_recv_frame_cb(&hi->ext, hi->hdlc_frame, hdlc_frame_len);
            }
            rx_ack_cleanup(hi);
        } break;
        case YAHDLC_FRAME_UI:
            hdlc_stat.ui_rx++;
            hdlc_os_exit_critical_section(&hi->ext);
            hdlc_recv_frame_cb(&hi->ext, hi->hdlc_frame, hdlc_frame_len);
            break;
        case YAHDLC_FRAME_ACK:
            hdlc_stat.rx_ack++;
            rx_ack(hi, hi->yahdlc.control.recv_seq_no);
            dbg_validate_state(hi, __FUNCTION__);
            hdlc_os_exit_critical_section(&hi->ext);
            rx_ack_cleanup(hi);
            break;
        case YAHDLC_FRAME_NACK:
            hdlc_stat.rx_nack++;

            rx_ack(hi, hi->yahdlc.control.recv_seq_no);
            // Currently retransmission is only triggered by timeout to keep
            // things simple.
            hdlc_os_exit_critical_section(&hi->ext);
            break;
        case YAHDLC_FRAME_SABM:
            send_ua_frame(hi);
            if (hi->dlc.state == ACTIVE) {
                reset(hi, HDLC_RESET_CAUSE_PEER_INITIATED);
                // reset will drop the mutex
                break;
            }
            // Also accept SABM as confirmation that peer has reset.
            // Fall through
        case YAHDLC_FRAME_UA:
            if (hi->dlc.state == RST_REQUIRED) {
                log_info("Got UA/SABM. TX reset complete");
                hi->dlc.state = RST_COMPLETE;
                hdlc_os_exit_critical_section(&hi->ext);
                hdlc_connected_cb(&hi->ext);
            } else {
                hdlc_os_exit_critical_section(&hi->ext);
            }
            break;
        default:
            hdlc_os_exit_critical_section(&hi->ext);
            break;
        }

    label_continue: // unlock mutex before goto!
        buf += res;
        assert((int)count >= res);
        count -= (uint32_t)res;
    } while (count);

    // Send maximum a single nack per received data chunk
    if (need_nack) {
        hdlc_os_enter_critical_section(&hi->ext);
        send_nack_frame(hi);
        hdlc_os_exit_critical_section(&hi->ext);
    }
}

#ifdef _MSC_BUILD
#pragma warning(push)
#pragma warning(disable : 4100) // reason not used if no logging defined
#endif

// Some error condition or timeout has been detected. Re-init everything. Mutex
// must be held when calling and will be dropped by this!
static void reset(hdlc_intdata_t *hi, hdlc_reset_cause_t cause)
{
    static const char *const reasons[] = {
        "application free",   // HDLC_RESET_CAUSE_APPLICATION_FREE
        "link lost",          // HDLC_RESET_CAUSE_LINK_LOST
        "keep-alive timeout", // HDLC_RESET_CAUSE_TIMEOUT_KEEP_ALIVE
        "retrans timeout",    // HDLC_RESET_CAUSE_TIMEOUT_RETRANSMIT
        "peer initiated",     // HDLC_RESET_CAUSE_PEER_INITIATED
    };
    static_assert(sizeof(reasons) / sizeof(reasons[0]) == HDLC_RESET_CAUSE_NUMBER_OF_CAUSES, "reasons array size mismatch");

    log_info("HDLC reset (%s)!", reasons[cause]);

    struct txq_tailhead temp_freeq = hi->dlc.txq;
    hdlc_reset(hi); // state = RST_REQUIRED

    if (cause == HDLC_RESET_CAUSE_PEER_INITIATED) {
        hi->dlc.state = RST_COMPLETE_WAIT;
        // timer already started in hdcl_reset()
    }

    hdlc_stat.reset++;

    // Drop mutex, so that application can call hdlc_send_frame() from here
    // (will fail but not block).
    hdlc_os_exit_critical_section(&hi->ext);
    hdlc_reset_cb(&hi->ext, cause);

    struct txq_entry *fe = TAILQ_FIRST(&temp_freeq);
    while (fe) {
        if (fe->frame) {
            hdlc_frame_sent_cb(&hi->ext, fe->frame, fe->len);
        }
        struct txq_entry *next = TAILQ_NEXT(fe, q);
        HDLC_OS_FREE(fe);
        fe = next;
    }
}

#ifdef _MSC_BUILD
#pragma warning(pop)
#endif

void hdlc_os_link_lost(hdlc_data_t *h)
{
    hdlc_intdata_t *hi = (hdlc_intdata_t *)h;
    hdlc_os_enter_critical_section(&hi->ext);
    reset(hi, HDLC_RESET_CAUSE_LINK_LOST);
    // reset will drop the mutex
}
