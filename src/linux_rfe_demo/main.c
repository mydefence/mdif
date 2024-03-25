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
 * Copyright 2024 MyDefence A/S.                                               *
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

void send_frame(const uint8_t *frame, uint32_t len);
void print_help(void);

//////////////////////////////////////////////////////////////////////////////
// Command line parsing (using argp)

static char doc[]                   = "Demo of communication with MyDefence device over MDIF HDLC/serial connection.\n\n"
                                      "Without options it will query the device info\n\n"
                                      "  [serial device]            Path to character device, e.g. \"/dev/ttyUSB0\"\n";
static char args_doc[]              = "[serial device]";
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
    printf("Connected\n\n");

    print_help();

    uint32_t size;
    uint8_t *req = encode_core_get_device_info_req(&size);
    send_frame(req, size);

    req = encode_rfe_get_state_info_req(&size);
    send_frame(req, size);
}

//////////////////////////////////////////////////////////////////////////////
// Main program logic

void send_frame(const uint8_t *frame, uint32_t len) {
    int ret = hdlc_send_frame(hdlc, frame, len);
    if (ret != 0) {
        printf("ERROR sending frame: %d\n\n", ret);
    }
}

void print_help(void) {
    printf("HELP - Press key + <enter>\n");
    printf(" h - help\n");
    printf(" q - quit\n");
    printf(" 1 - start/add 2G4\n");
    printf(" 2 - start/add 5G8\n");
    printf(" 3 - start/add 5G2\n");
    printf(" 4 - start/add 2G4 + 5G8\n");
    printf(" s - stop\n");
    printf(" i - get device info\n");
    printf(" I - get state info\n");
    printf(" b - get battery status\n");
    printf(" r - reset\n");
    printf("\n");
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

    int fd = serial_open(args.serial_device);

    printf("Connecting ...\n\n");

    hdlc_linux_init(); // calls hdlc_init()
    start_rx_thread(fd);

    uint32_t size;
    uint8_t *req;
    while (1) {
        int ch = getchar();
        switch (ch) {
        case 'h':
            print_help();
            break;
        case 'q':
            printf("Exiting\n");
            return 0;

        case '1':
            req = encode_rfe_start_req(&size, false, 1, (Mdif__Rfe__FreqBand[]){MDIF__RFE__FREQ_BAND__FREQ_BAND_2_4_GHZ});
            send_frame(req, size);
            break;

        case '2':
            req = encode_rfe_start_req(&size, false, 1, (Mdif__Rfe__FreqBand[]){MDIF__RFE__FREQ_BAND__FREQ_BAND_5_8_GHZ});
            send_frame(req, size);
            break;

        case '3':
            req = encode_rfe_start_req(&size, false, 1, (Mdif__Rfe__FreqBand[]){MDIF__RFE__FREQ_BAND__FREQ_BAND_5_2_GHZ});
            send_frame(req, size);
            break;

        case '4':
            req = encode_rfe_start_req(&size, false, 2, (Mdif__Rfe__FreqBand[]){MDIF__RFE__FREQ_BAND__FREQ_BAND_2_4_GHZ, MDIF__RFE__FREQ_BAND__FREQ_BAND_5_8_GHZ});
            send_frame(req, size);
            break;

        case 's':
            req = encode_rfe_stop_req(&size);
            send_frame(req, size);
            break;

        case 'i':
            req = encode_core_get_device_info_req(&size);
            send_frame(req, size);
            break;

        case 'I':
            req = encode_rfe_get_state_info_req(&size);
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
    }
}
