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
 * Program/file : java_port.c                                                  *
 *                                                                             *
 * Description  : Java implementation of OS abstraction interface for hdlc     *
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
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "hdlc/include/hdlc.h"
#include "hdlc/include/hdlc_os.h"
#include "hdlc_port.h"
#include "java_port.h"

#define SIG SIGRTMIN

static pthread_t timer_thread;
static pthread_mutex_t hdlc_mutex;

static struct itimerspec its_start = {
    .it_value.tv_sec = 0,
    .it_value.tv_nsec = 0, // set in init
    .it_interval.tv_sec = 0,
    .it_interval.tv_nsec = 0,
};
static struct itimerspec its_stop = {
    .it_value.tv_sec = 0,
    .it_value.tv_nsec = 0,
    .it_interval.tv_sec = 0,
    .it_interval.tv_nsec = 0,
};
static int timeout_fd;

static void *timer_thread_func(void *ptr);

// This port only supports single instance of hdlc. This instance data must be
// used on all calls to hdlc functions. It will be valid after hdlc_java_init().
hdlc_data_t *hdlc;

void hdlc_java_init()
{
    its_start.it_value.tv_nsec = JAVA_HDLC_TIMEOUT_MS * 1000000;

    if (pthread_mutex_init(&hdlc_mutex, NULL) != 0) {
        log_fatal("mutex init has failed");
        exit(1);
    }

    timeout_fd = timerfd_create(CLOCK_REALTIME, 0);
    if (timeout_fd == -1) {
        perror("timerfd_create");
        exit(1);
    }
    if (pthread_create(&timer_thread, NULL, timer_thread_func, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }

    hdlc = hdlc_init(NULL);
}

static void *timer_thread_func(void *ptr)
{
    while (1) {
        uint64_t exp;

        if (read(timeout_fd, &exp, sizeof(uint64_t)) == -1) {
            perror("read");
            exit(1);
        }

        if (hdlc) {
            hdlc_os_timeout(hdlc);
        } else {
            // This is possible during startup if hdlc_init() has not yet
            // returned. Try again later.
            log_warn("hdlc not initialized (timeout)");
            hdlc_os_start_timer(NULL);
        }
    }
}

void hdlc_os_start_timer(hdlc_data_t *_hdlc)
{
    if (timerfd_settime(timeout_fd, 0, &its_start, NULL) == -1) {
        perror("timerfd_settime (start)");
        exit(1);
    }
}

void hdlc_os_stop_timer(hdlc_data_t *_hdlc)
{
    if (timerfd_settime(timeout_fd, 0, &its_stop, NULL) == -1) {
        perror("timerfd_settime (stop)");
        exit(1);
    }
}

void hdlc_os_enter_critical_section(hdlc_data_t *hdlc)
{
    pthread_mutex_lock(&hdlc_mutex);
}

void hdlc_os_exit_critical_section(hdlc_data_t *hdlc)
{
    pthread_mutex_unlock(&hdlc_mutex);
}
