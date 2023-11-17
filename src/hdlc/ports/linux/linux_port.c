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
 * Program/file : linux_port.c                                                 *
 *                                                                             *
 * Description  : Linux implementation of OS abstraction interface for hdlc    *
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
#include "linux_port.h"

#define SIG SIGRTMIN

static pthread_t rx_thread, timer_thread;
static pthread_mutex_t hdlc_mutex;

enum rx_thread_running_t rx_thread_running;

int hdlc_socket = -1;

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
// used on all calls to hdlc functions. It will be valid after hdlc_linux_init().
hdlc_data_t *hdlc;

void hdlc_linux_init()
{
    its_start.it_value.tv_nsec = LINUX_HDLC_TIMEOUT_MS * 1000000;

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

void *rx_thread_func(void *ptr)
{
    uint8_t buf[HDLC_MAX_FRAME_LEN];
    rx_thread_running = RX_THREAD_RUNNING;
    while (1) {
        // Use select to block until data is available (because socket is O_NONBLOCK)
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(hdlc_socket, &readfds);
        int ret = select(hdlc_socket + 1, &readfds, NULL, NULL, NULL);
        if (ret == -1) {
            perror("select");
            exit(1);
        }
        assert(ret == 1);
        assert(FD_ISSET(hdlc_socket, &readfds));

        // Read data from socket
        ret = read(hdlc_socket, buf, sizeof(buf));
        if (ret == -1) {
            // Peer process exited, maybe normal script termination
            log_warn("Connection lost");
            perror("read");
            break;
        }
        if (ret == 0) {
            log_warn("peer exited");
            break;
        }

#ifdef STRESS_TEST
        ret = stress_test_read_cb(buf, ret);
        if (!ret) {
            continue;
        }
#endif

        hdlc_os_rx(hdlc, buf, ret);
    }

    rx_thread_running = RX_THREAD_STOPPED;
    return NULL;
}

// socket is assumed to be non-blocking, because block in hold_os_tx() can cause dead-lock.
void start_rx_thread(int socket)
{
    hdlc_socket = socket;
    rx_thread_running = RX_THREAD_INIT;
    if (pthread_create(&rx_thread, NULL, rx_thread_func, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }

    // Poll for rx_thread to start to avoid some races
    while (rx_thread_running == RX_THREAD_INIT) {
        usleep(1000);
    }
}

// Write to socket. Since we throttle data to HDLC we do not expect failure of
// OS write(). If it happens we let HDLC retransmit the frame.
int hdlc_os_tx(hdlc_data_t *_hdlc, const uint8_t *buf, uint32_t count)
{
    if (hdlc_socket < 0) {
        log_info("hdlc_socket not initialized");
        return 0;
    }

#ifdef STRESS_TEST
    int ret = stress_test_write(hdlc_socket, buf, count);
#else
    int ret = write(hdlc_socket, buf, count);
#endif

    log_info("tx %d bytes", ret);
    if (ret != count) {
        if (ret == -1) {
            if (errno == EAGAIN || errno == EPIPE) {
                log_warn("write failed: %s. Discarding frame", strerror(errno));
                return 0;
            }
            perror("write");
            exit(1);
        } else {
            log_warn("write incomplete. returned %d, expected %d", ret, count);
        }
    }

    return ret;
}

// Call this if nothing else to do, to keep rx thread running. Returns in case
// of link loss.
void run_threads()
{
    pthread_join(rx_thread, NULL);
}

static void *timer_thread_func(void *ptr)
{
    while (1) {
        uint64_t exp;

        if (read(timeout_fd, &exp, sizeof(uint64_t)) == -1) {
            perror("read");
            exit(1);
        }

        hdlc_os_timeout(hdlc);
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

int serial_open(const char *serial_device)
{
    int fd = open(serial_device, O_RDWR);
    if (fd < 0) {
        perror("open serial port");
        exit(1);
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) < 0) {
        perror("tcgetattr");
        exit(1);
    }
    tty.c_cflag &= ~PARENB;                                                      // Clear parity bit
    tty.c_cflag &= ~CSIZE;                                                       // Clear all the size bits
    tty.c_cflag |= CS8;                                                          // 8 bits per byte
    tty.c_cflag &= ~CSTOPB;                                                      // Clear stop field, only one stop bit used in communication
    tty.c_cflag &= ~CRTSCTS;                                                     // Disable RTS/CTS hardware flow control
    tty.c_cflag |= CREAD | CLOCAL;                                               // Turn on READ & ignore ctrl lines
    tty.c_lflag &= ~ICANON;                                                      // Disable canonical mode (waiting for line feed)
    tty.c_lflag &= ~ECHO;                                                        // Disable echo
    tty.c_lflag &= ~ECHOE;                                                       // Disable erasure
    tty.c_lflag &= ~ECHONL;                                                      // Disable new-line echo
    tty.c_lflag &= ~ISIG;                                                        // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);                                      // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes
    tty.c_oflag &= ~OPOST;                                                       // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR;                                                       // Prevent conversion of newline to carriage return/line feed
    tty.c_cc[VTIME] = 0;                                                         // No blocking, return immediately with what is available
    tty.c_cc[VMIN] = 1;                                                          // No blocking, return immediately as soon as any data is available
    cfsetispeed(&tty, B460800);
    cfsetospeed(&tty, B460800);
    if (tcsetattr(fd, TCSANOW, &tty) < 0) {
        perror("tcsetattr");
    }

    // Set exclusive access
    if (-1 == ioctl(fd, TIOCEXCL)) {
        perror("ioctl - TIOCEXCL");
        exit(1);
    }

    return fd;
}
