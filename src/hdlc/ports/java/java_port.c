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

typedef struct hdlc_instance_t {
    hdlc_data_t *hdlc;
    uint32_t instance_id;
    struct itimerspec its_start;
    struct itimerspec its_stop;
    pthread_t timer_thread;
    pthread_mutex_t hdlc_mutex;
    int8_t shutdown;
    int timeout_fd;
} hdlc_instance_t;

static void *timer_thread_func(void *inst);

static hdlc_instance_t hdlc_instance_data[HDLC_NUM_INSTANCES] = {0};

void hdlc_java_init(uint32_t instance_id)
{
    if (instance_id >= HDLC_NUM_INSTANCES) {
        log_error("hdlc instance %d is not supported", instance_id);
        return; // Invalid instance ID
    }
    hdlc_instance_t *inst = &hdlc_instance_data[instance_id];
    inst->shutdown = 0;
    if (inst->hdlc == NULL) {
        log_info("hdlc instance %d initializing", instance_id);
        inst->its_start.it_value.tv_nsec = JAVA_HDLC_TIMEOUT_MS * 1000000;
        inst->instance_id = instance_id;

        if (pthread_mutex_init(&inst->hdlc_mutex, NULL) != 0) {
            log_fatal("mutex init has failed");
            exit(1);
        }

        inst->timeout_fd = timerfd_create(CLOCK_REALTIME, 0);
        if (inst->timeout_fd == -1) {
            perror("timerfd_create");
            exit(1);
        }
        if (pthread_create(&inst->timer_thread, NULL, timer_thread_func, inst) != 0) {
            perror("pthread_create");
            exit(1);
        }
        // timer_thread_func may start before hdlc_init() returns in which case
        // inst->hdlc won't be set yet. This is handled in the timer thread
        inst->hdlc = hdlc_init(inst);
    } else {
        log_info("hdlc re-starting");
        // hdlc_os_link_lost() will restart the timer
        hdlc_os_link_lost(inst->hdlc);
    }
}

void hdlc_java_stop(uint32_t instance_id)
{
    hdlc_instance_t *inst = &hdlc_instance_data[instance_id];
    inst->shutdown = 1; // Exit timer thread loop
    hdlc_os_stop_timer(inst->hdlc);
}

static void *timer_thread_func(void *ptr)
{
    hdlc_instance_t *inst = (hdlc_instance_t *)ptr;
    while (1) {
        uint64_t exp;

        if (read(inst->timeout_fd, &exp, sizeof(uint64_t)) == -1) {
            perror("read");
            exit(1);
        }

        if (inst->shutdown == 1) {
            continue;
        }

        if (inst->hdlc) {
            hdlc_os_timeout(inst->hdlc);
        } else {
            // This is possible during startup if hdlc_init() has not yet
            // returned. Try again later.
            log_warn("hdlc not initialized (timeout)");
            hdlc_os_start_timer(inst->hdlc);
        }
    }
}

void hdlc_os_start_timer(hdlc_data_t *hdlc)
{
    hdlc_instance_t *inst = (hdlc_instance_t *)hdlc->user_data;
    if (timerfd_settime(inst->timeout_fd, 0, &inst->its_start, NULL) == -1) {
        perror("timerfd_settime (start)");
    }
}

void hdlc_os_stop_timer(hdlc_data_t *hdlc)
{
    hdlc_instance_t *inst = (hdlc_instance_t *)hdlc->user_data;
    if (timerfd_settime(inst->timeout_fd, 0, &inst->its_stop, NULL) == -1) {
        perror("timerfd_settime (stop)");
    }
}

void hdlc_os_enter_critical_section(hdlc_data_t *hdlc)
{
    hdlc_instance_t *inst = (hdlc_instance_t *)hdlc->user_data;
    pthread_mutex_lock(&inst->hdlc_mutex);
}

void hdlc_os_exit_critical_section(hdlc_data_t *hdlc)
{
    hdlc_instance_t *inst = (hdlc_instance_t *)hdlc->user_data;
    pthread_mutex_unlock(&inst->hdlc_mutex);
}

hdlc_data_t *get_hdlc_data(uint32_t instance_id)
{
    if (instance_id >= HDLC_NUM_INSTANCES) {
        log_error("hdlc instance %d is not supported", instance_id);
        return NULL;
    }
    return hdlc_instance_data[instance_id].hdlc;
}

uint32_t get_hdlc_instance_id(hdlc_data_t *hdlc)
{
    return ((hdlc_instance_t *)hdlc->user_data)->instance_id;
}