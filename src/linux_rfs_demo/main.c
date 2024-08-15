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
 * Program/file : main.c                                                       *
 *                                                                             *
 * Description  : Provide the user an interface to prompt protobuf messages to *
 *              : the device running mdif over serial connection using hdlc.   *
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
#include <argp.h>
#include <stdlib.h>

#include "hdlc/include/hdlc.h"
#include "hdlc/include/hdlc_os.h"
#include "hdlc/ports/linux/linux_port.h"

#include "codec.h"
#include "linux_mdif_socket/mdif_socket.h"

void send_frame(const uint8_t *frame, uint32_t len);

//////////////////////////////////////////////////////////////////////////////
// Command line parsing (using argp)

static char doc[]                   = "\nDemo of communication with MyDefence device over MDIF TCP or HDLC/serial connection.\n\n"
                                      "  [device]            Address of MDIF device, e.g. \"/dev/ttyUSB0\"\n"
                                      "                      or \"192.168.1.42\"\n";
static char args_doc[]              = "[device]";
static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, "Verbose output, e.g. decoding of messages. Repeat for increased verbosity."},
    {0}};

struct args {
    const char *serial_device; // first positional argument
    int verbose;
} args = {
    // Defaults
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct args *args = state->input;
    switch (key) {
    case ARGP_KEY_ARG:
        if (state->arg_num >= 1)
            /* Too many args. */
            argp_usage(state);

        args->serial_device = arg;
        break;

    case ARGP_KEY_END:
        if (state->arg_num < 1)
            /* Not enough args. */
            argp_usage(state);
        break;

    case 'v':
        args->verbose++;
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

//////////////////////////////////////////////////////////////////////////////
// Implementation of HDLC callbacks

void hdlc_frame_sent_cb(hdlc_data_t *_hdlc, const uint8_t *frame, uint32_t len) {
    if (args.verbose) {
        printf("hdlc transferred frame %d bytes\n", len);
    }
    free((uint8_t *)frame);
}

void hdlc_recv_frame_cb(hdlc_data_t *_hdlc, uint8_t *frame, uint32_t len) {
    if (args.verbose) printf("hdlc recv frame %d bytes\n", len);
    decode_mdif_msg(frame, len);
}

void hdlc_reset_cb(hdlc_data_t *_hdlc, hdlc_reset_cause_t cause) {
    if (args.verbose) printf("hdlc reset (%d)\n", cause);
}

void hdlc_connected_cb(hdlc_data_t *hdlc) {
    printf("hdlc connected\n");

    uint32_t size;
    uint8_t *req = encode_core_get_device_info_req(&size);
    send_frame(req, size);

    req = encode_rfs_start_req(&size);
    send_frame(req, size);
}

//////////////////////////////////////////////////////////////////////////////
// Main program logic

void send_frame(const uint8_t *frame, uint32_t len) {
    if (mdif_socket != -1) {
        mdif_socket_send(frame, len);
        free((uint8_t*)frame);
    } else {
        int ret = hdlc_send_frame(hdlc, frame, len);
        if (ret != 0) {
            printf("ERROR sending frame: %d\n\n", ret);
        }
        // free'd in hdlc_frame_sent_cb()
    }
}

int main(int argc, char **argv) {
    argp_parse(&argp, argc, argv, 0, 0, &args);
    // HDLC related logging
    int hdlc_log_level;
    if (args.verbose >= 2) {
        hdlc_log_level = LOG_DEBUG;
    } else if (args.verbose) {
        hdlc_log_level = LOG_WARN;
    } else {
        hdlc_log_level = LOG_ERROR;
    };
    log_set_level(hdlc_log_level);

    if (args.serial_device[0] == '/') {
        int fd = serial_open(args.serial_device);
        hdlc_linux_init(); // calls hdlc_init()
        start_rx_thread(fd);
    } else {
        mdif_socket_init(args.serial_device);
    }

    uint32_t size;
    uint8_t *req;
    int ch = 'h';
    while (1) {
        switch (ch) {
        case 'h':
            printf("HELP - Press key + <enter>\n");
            printf(" h - help\n");
            printf(" q - quit\n");
            printf(" s - start\n");
            printf(" S - stop\n");
            printf(" i - get info\n");
            printf(" b - get battery status\n");
            printf(" r - reset\n");
            printf("------------Drone info cmd.\n");
            printf("The following examples allows the user to see\n");
            printf("how the drone info is accessed from, start,\n");
            printf("middle, end, and unknown type_id.\n");
            printf(" d - get drone info with type_id=0 - the first in list\n");
            printf(" D - get drone info with type_id=-1 - the last in list\n");
            printf(" m - get drone info with type_id=1000404 - somewhere within in list\n");
            printf(" M - get drone info with corrupted type_id\n");
            printf("------------\n");
            printf("\n");
            break;
        case 'q':
            printf("Exiting\n");
            return 0;

        case 's':
            req = encode_rfs_start_req(&size);
            send_frame(req, size);
            break;

        case 'S':
            req = encode_rfs_stop_req(&size);
            send_frame(req, size);
            break;

        case 'd':
            // Query First.
            req = encode_rfs_get_drone_info_req(&size, 0);
            send_frame(req, size);
            break;

        case 'D':
            // Query Last of threat_info[max]
            req = encode_rfs_get_drone_info_req(&size, 0xFFFFFFFF);
            send_frame(req, size);
            break;

        case 'm':
            // Query middle.
            req = encode_rfs_get_drone_info_req(&size, 1000404);
            send_frame(req, size);
            break;

        case 'M':
            // Query unknown
            req = encode_rfs_get_drone_info_req(&size, 1000405);
            send_frame(req, size);
            break;

        case 'i':
            req = encode_core_get_device_info_req(&size);
            send_frame(req, size);
            break;

        case 'b':
            req = encode_core_get_battery_status_req(&size);
            send_frame(req, size);
            break;
        case 'r':
            req = encode_core_reset_req(&size);
            send_frame(req, size);
            break;
        }
        ch = getchar();
    }
}
