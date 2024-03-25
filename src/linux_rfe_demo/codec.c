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
 * Program/file : codec.c                                                      *
 *                                                                             *
 * Description  :  This file is provided to help with 3rd party integration.   *
 *              :  It provides an example of MDIF encoding and decoding.       *
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

/*******************************************************************************
 *                                Include files
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "codec.h"


/*******************************************************************************
 *                               Macro definitions
 *******************************************************************************/
// See the output *.pb-c.h from input *.proto file.

/*******************************************************************************
 *                      Enumerations/Type definitions/Structs
 *******************************************************************************/
// See the output *.pb-c.h from input *.proto file.

/*******************************************************************************
 *                             Global variables/const
 *******************************************************************************/

/*******************************************************************************
 *                             Local variables/const
 *******************************************************************************/

/*******************************************************************************
 *                           Local Function prototypes
 *******************************************************************************/
static void print_rfe_state(Mdif__Rfe__RfeState *rfe_state);
static void print_state_info(Mdif__Rfe__StateInfo *state_info);
static decode_rtn_t decode_rfe(const uint8_t *buf, uint32_t size);

/*******************************************************************************
 *                                 Implementation
 *******************************************************************************/

/*******************************************
 * Client sends encoded messages to device *
 *******************************************/

/**
 * Encodes a RFE Start request message.
 *
 * This function constructs a RFE Start request message and returns the encoded
 * message along with its size.
 *
 * @param size Pointer to a variable where the size of the encoded message will
 *             be stored.
 * @param clear_list Boolean to clear the current frequency band list.
 * @param n_freq_band_list Number of frequency bands in the list.
 * @param freq_band_list List of frequency bands to start.
 * @return A dynamically allocated uint8_t pointer containing the encoded
 *         message. The caller is responsible for freeing the memory when done.
*/
uint8_t *encode_rfe_start_req(uint32_t *size, bool clear_list, size_t n_freq_band_list, Mdif__Rfe__FreqBand *freq_band_list) {
    // Construct the payload
    Mdif__Rfe__StartReq start_req = MDIF__RFE__START_REQ__INIT;
    start_req.clear_list = clear_list;
    start_req.n_freq_band_list = n_freq_band_list;
    start_req.freq_band_list = freq_band_list;

    // Construct the message
    Mdif__Rfe__RfeMsg rfe_msg  = MDIF__RFE__RFE_MSG__INIT;
    rfe_msg.msg_case           = MDIF__RFE__RFE_MSG__MSG_START_REQ;
    rfe_msg.start_req = &start_req;

    // Memory allocation sizing
    *size        = mdif__rfe__rfe_msg__get_packed_size(&rfe_msg);
    uint8_t *msg = malloc(*size);
    mdif__rfe__rfe_msg__pack(&rfe_msg, msg);
    return msg;
}

/**
 * Encodes a RFE Stop request message.
 *
 * This function constructs a RFE Stop request message and returns the encoded
 * message along with its size.
 *
 * @param size Pointer to a variable where the size of the encoded message will
 *             be stored.
 * @return A dynamically allocated uint8_t pointer containing the encoded
 *         message. The caller is responsible for freeing the memory when done.
*/
uint8_t *encode_rfe_stop_req(uint32_t *size) {
    // Construct the payload
    Mdif__Rfe__StopReq stop_req = MDIF__RFE__STOP_REQ__INIT;

    // Construct the message
    Mdif__Rfe__RfeMsg rfe_msg  = MDIF__RFE__RFE_MSG__INIT;
    rfe_msg.msg_case           = MDIF__RFE__RFE_MSG__MSG_STOP_REQ;
    rfe_msg.stop_req = &stop_req;

    // Memory allocation sizing
    *size        = mdif__rfe__rfe_msg__get_packed_size(&rfe_msg);
    uint8_t *msg = malloc(*size);
    mdif__rfe__rfe_msg__pack(&rfe_msg, msg);
    return msg;
}

/**
 * Encodes a RFE Get State Info request message.
 *
 * This function constructs a RFE Get State Info request message and returns the
 * encoded message along with its size.
 *
 * @param size Pointer to a variable where the size of the encoded message will
 *             be stored.
 * @return A dynamically allocated uint8_t pointer containing the encoded
 *         message. The caller is responsible for freeing the memory when done.
*/
uint8_t *encode_rfe_get_state_info_req(uint32_t *size) {
    // Construct the payload
    Mdif__Rfe__GetStateInfoReq get_state_info_req = MDIF__RFE__GET_STATE_INFO_REQ__INIT;

    // Construct the message
    Mdif__Rfe__RfeMsg rfe_msg  = MDIF__RFE__RFE_MSG__INIT;
    rfe_msg.msg_case           = MDIF__RFE__RFE_MSG__MSG_GET_STATE_INFO_REQ;
    rfe_msg.get_state_info_req = &get_state_info_req;

    // Memory allocation sizing
    *size        = mdif__rfe__rfe_msg__get_packed_size(&rfe_msg);
    uint8_t *msg = malloc(*size);
    mdif__rfe__rfe_msg__pack(&rfe_msg, msg);
    return msg;
}

/****************************************************
 * Client receives and decodes messages from device *
 ****************************************************/

/**
 *
 * This function tries all decode methods. A fallthrough will results
 * in a decode error.
 *
 * @param buf Pointer to the binary buffer containing the xxxx Message.
 * @param size Size of the binary buffer.
 * @return A decode result code indicating the outcome of the decoding process.
 *         Possible return values are DECODE_SUCCESS for successful decoding,
 *         DECODE_ERR_NO_DECODER if the message type is not set, and DECODE_ERR_OTHER
 *         for other decoding errors.
 */
decode_rtn_t decode_mdif_msg(const uint8_t *buf, uint32_t size) {
    if (!buf) {
        return DECODE_ERR_OTHER;
    }

    decode_rtn_t rtn;
    if ((rtn = decode_rfe(buf, size)) != DECODE_ERR_NO_DECODER) {
        return rtn;
    }
    if ((rtn = decode_core(buf, size)) != DECODE_ERR_NO_DECODER) {
        return rtn;
    }

    return DECODE_ERR_NO_DECODER;
}

/**
 * Decode a RFE Message from a binary buffer.
 *
 * This function decodes a binary buffer containing a RFE Message and processes
 * the decoded message based on its type. It prints information related to the
 * decoded message for debugging purposes and returns a decode result code.
 *
 * @param buf Pointer to the binary buffer containing the RFE Message.
 * @param size Size of the binary buffer.
 * @return A decode result code indicating the outcome of the decoding process.
 *         Possible return values are DECODE_SUCCESS for successful decoding,
 *         DECODE_ERR_NO_DECODER if the message type is not set, and DECODE_ERR_OTHER
 *         for other decoding errors.
 */
static decode_rtn_t decode_rfe(const uint8_t *buf, uint32_t size) {
    Mdif__Rfe__RfeMsg *rfe_msg = mdif__rfe__rfe_msg__unpack(NULL, size, buf);

    // Was unpack successful?
    if (rfe_msg == NULL) {
        return DECODE_ERR_NO_DECODER;
    }

    // Default
    decode_rtn_t rtn = DECODE_SUCCESS;

    // Switch through options described in rfs.proto RfsMsg
    switch (rfe_msg->msg_case) {

    // Device does not have response to target.
    case MDIF__RFE__RFE_MSG__MSG__NOT_SET: {
        rtn = DECODE_ERR_NO_DECODER;
        break;
    }

    // Device responds to client
    case MDIF__RFE__RFE_MSG__MSG_START_RES: {
        printf("Decode START_RES\n");
        printf("    status=%d\n", rfe_msg->start_res->status);
        for (size_t i = 0; i < rfe_msg->start_res->n_tx_status_list; i++) {
            printf("    tx_status_list[%zu]=%d\n", i, rfe_msg->start_res->tx_status_list[i]);
        }
        break;
    }

    case MDIF__RFE__RFE_MSG__MSG_STOP_RES: {
        printf("Decode STOP_RES\n");
        printf("    status=%u\n", rfe_msg->stop_res->status);
        break;
    }

    case MDIF__RFE__RFE_MSG__MSG_GET_STATE_INFO_RES: {
        printf("Decode GET_STATE_INFO_RES\n");
        print_state_info(rfe_msg->get_state_info_res);
        break;
    }
    case MDIF__RFE__RFE_MSG__MSG_STATE_INFO_IND: {
        printf("Decode STATE_INFO_IND\n");
        print_state_info(rfe_msg->state_info_ind);
        break;
    }

    default:
        printf("Rfe message not decoded: %u", rfe_msg->msg_case);
        rtn = DECODE_ERR_OTHER;
    }

    mdif__rfe__rfe_msg__free_unpacked(rfe_msg, NULL);

    printf("\n");

    return rtn;
}

static void print_rfe_state(Mdif__Rfe__RfeState *rfe_state) {
    printf("        controller_type=%u\n", rfe_state->controller_type);
    printf("        battery_status=%u\n", rfe_state->battery_status);
    printf("        temperature_status=%u\n", rfe_state->temperature_status);
    printf("        config\n");
    printf("            automatic=%u\n", (bool)(rfe_state->config & MDIF__RFE__CONFIG__CONFIG_AUTOMATIC));
    printf("            shutdown=%u\n", (bool)(rfe_state->config & MDIF__RFE__CONFIG__CONFIG_SHUTDOWN));
    printf("            transmitting=%u\n", (bool)(rfe_state->config & MDIF__RFE__CONFIG__CONFIG_TRANSMITTING));
    printf("            tx_gnss=%u\n", (bool)(rfe_state->config & MDIF__RFE__CONFIG__CONFIG_TX_GNSS));
    printf("            tx_2g4=%u\n", (bool)(rfe_state->config & MDIF__RFE__CONFIG__CONFIG_TX_2G4));
    printf("            tx_5g2=%u\n", (bool)(rfe_state->config & MDIF__RFE__CONFIG__CONFIG_TX_5G2));
    printf("            tx_5g8=%u\n", (bool)(rfe_state->config & MDIF__RFE__CONFIG__CONFIG_TX_5G8));
    printf("            low_power=%u\n", (bool)(rfe_state->config & MDIF__RFE__CONFIG__CONFIG_LOW_POWER));
}
static void print_state_info(Mdif__Rfe__StateInfo *state_info) {
    if (state_info->local != NULL) {
        printf("    local\n");
        print_rfe_state(state_info->local);
    } else {
        printf("    local=NULL\n");
    }

    if (state_info->remote != NULL) {
        printf("    remote\n");
        print_rfe_state(state_info->remote);
    } else {
        printf("    remote=NULL\n");
    }
}