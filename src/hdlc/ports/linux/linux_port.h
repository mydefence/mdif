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
 * Program/file : linux_port.h                                                 *
 *                                                                             *
 * Description  : Linux port (OS abstraction or integration) interface to      *
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
#ifndef _LINUX_PORT_H_
#define _LINUX_PORT_H_

// This port only supports a single instance, so using various global variables
// instead of hdlc user_data pointer.

#include "hdlc/include/hdlc_os.h"

#if defined STRESS_TEST
extern unsigned stress_test_hdlc_timeout_ms;
#define LINUX_HDLC_TIMEOUT_MS stress_test_hdlc_timeout_ms
#elif !defined LINUX_HDLC_TIMEOUT_MS
#define LINUX_HDLC_TIMEOUT_MS 200
#endif

enum rx_thread_running_t {
    RX_THREAD_INIT,
    RX_THREAD_RUNNING,
    RX_THREAD_STOPPED,
};

extern enum rx_thread_running_t rx_thread_running;

void start_rx_thread(int socket);
void run_threads();
void hdlc_linux_init();
void *rx_thread_func(void *ptr);

#ifdef HDLC_READ_CB
uint16_t hdlc_read_cb(uint8_t *frame, uint16_t len);
#endif
#ifdef HDLC_WRITE
int hdlc_write(int hdlc_socket, const uint8_t *buf, uint16_t count);
#endif

// This port only supports single instance of hdlc. This instance data must be
// used on all calls to hdlc functions. It will be valid after hdlc_linux_init().
extern hdlc_data_t *hdlc;

extern int hdlc_socket;

// Helper function to open serial device, and configure baud rate etc.
int serial_open(const char *serial_device);

#endif // _LINUX_PORT_H_
