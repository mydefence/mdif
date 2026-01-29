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

/*******************************************************************************
 *                                Include files
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

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
static decode_rtn_t decode_rfs(const uint8_t *buf, uint32_t size);

/*******************************************************************************
 *                                 Implementation
 *******************************************************************************/

/*******************************************
 * Client sends encoded messages to device *
 *******************************************/

/**
 * Encodes a RFS Get Drone Info request message.
 *
 * This function constructs a RFS Get Drone Info request message and returns the
 * encoded message along with its size.
 *
 * @param size Pointer to a variable where the size of the encoded message will
 *             be stored.
 * @param type_id The type_id of the drone to get information about.
 * @return A dynamically allocated uint8_t pointer containing the encoded
 *         message. The caller is responsible for freeing the memory when done.
*/
uint8_t *encode_rfs_get_drone_info_req(uint32_t *size, uint32_t type_id) {
    // Construct the payload
    Mdif__Rfs__GetDroneInfoReq get_drone_info_req = MDIF__RFS__GET_DRONE_INFO_REQ__INIT;

    // Construct the message
    Mdif__Rfs__RfsMsg rfs_msg  = MDIF__RFS__RFS_MSG__INIT;
    rfs_msg.msg_case           = MDIF__RFS__RFS_MSG__MSG_GET_DRONE_INFO_REQ;
    get_drone_info_req.type_id = type_id;
    rfs_msg.get_drone_info_req = &get_drone_info_req;

    // Memory allocation sizing
    *size        = mdif__rfs__rfs_msg__get_packed_size(&rfs_msg);
    uint8_t *msg = malloc(*size);
    mdif__rfs__rfs_msg__pack(&rfs_msg, msg);
    return msg;
}

/**
 * Encodes a RFS Start request message.
 *
 * This function constructs a RFS Start request message and returns the encoded
 * message along with its size.
 *
 * @param size Pointer to a variable where the size of the encoded message will
 *             be stored.
 * @return A dynamically allocated uint8_t pointer containing the encoded
 *         message. The caller is responsible for freeing the memory when done.
 */
uint8_t *encode_rfs_start_req(uint32_t *size) {
    // Construct the payload
    Mdif__Rfs__StartReq start_req = MDIF__RFS__START_REQ__INIT;

    // Construct the message
    Mdif__Rfs__RfsMsg rfs_msg = MDIF__RFS__RFS_MSG__INIT;
    rfs_msg.msg_case          = MDIF__RFS__RFS_MSG__MSG_START_REQ;
    rfs_msg.start_req         = &start_req;

    // Memory allocation sizing
    *size        = mdif__rfs__rfs_msg__get_packed_size(&rfs_msg);
    uint8_t *msg = malloc(*size);
    mdif__rfs__rfs_msg__pack(&rfs_msg, msg);
    return msg;
}

/**
 * Encodes a RFS Stop request message.
 *
 * This function constructs a RFS Stop request message and returns the encoded
 * message along with its size.
 *
 * @param size Pointer to a variable where the size of the encoded message will
 *             be stored.
 * @return A dynamically allocated uint8_t pointer containing the encoded
 *         message. The caller is responsible for freeing the memory when done.
 */
uint8_t *encode_rfs_stop_req(uint32_t *size) {
    // Construct the payload
    Mdif__Rfs__StopReq stop_req = MDIF__RFS__STOP_REQ__INIT;

    // Construct the message
    Mdif__Rfs__RfsMsg rfs_msg = MDIF__RFS__RFS_MSG__INIT;
    rfs_msg.msg_case          = MDIF__RFS__RFS_MSG__MSG_STOP_REQ;
    rfs_msg.stop_req          = &stop_req;

    // Memory allocation sizing
    *size        = mdif__rfs__rfs_msg__get_packed_size(&rfs_msg);
    uint8_t *msg = malloc(*size);
    mdif__rfs__rfs_msg__pack(&rfs_msg, msg);
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
    if ((rtn = decode_rfs(buf, size)) != DECODE_ERR_NO_DECODER) {
        return rtn;
    }
    if ((rtn = decode_core(buf, size)) != DECODE_ERR_NO_DECODER) {
        return rtn;
    }

    return DECODE_ERR_NO_DECODER;
}

/**
 * Decode a RFS Message from a binary buffer.
 *
 * This function decodes a binary buffer containing a RFS Message and processes
 * the decoded message based on its type. It prints information related to the
 * decoded message for debugging purposes and returns a decode result code.
 *
 * @param buf Pointer to the binary buffer containing the RFS Message.
 * @param size Size of the binary buffer.
 * @return A decode result code indicating the outcome of the decoding process.
 *         Possible return values are DECODE_SUCCESS for successful decoding,
 *         DECODE_ERR_NO_DECODER if the message type is not set, and DECODE_ERR_OTHER
 *         for other decoding errors.
 */
static decode_rtn_t decode_rfs(const uint8_t *buf, uint32_t size) {
    Mdif__Rfs__RfsMsg *rfs_msg = mdif__rfs__rfs_msg__unpack(NULL, size, buf);

    // Was unpack successful?
    if (rfs_msg == NULL) {
        return DECODE_ERR_NO_DECODER;
    }

    // Default
    decode_rtn_t rtn = DECODE_SUCCESS;

    // Switch through options described in rfs.proto RfsMsg
    switch (rfs_msg->msg_case) {

    // Device does not have response to target.
    case MDIF__RFS__RFS_MSG__MSG__NOT_SET: {
        rtn = DECODE_ERR_NO_DECODER;
        break;
    }

    // Device responds to client
    case MDIF__RFS__RFS_MSG__MSG_START_RES: {
        printf("Decode START_RES\n");
        printf("    status=%d\n", rfs_msg->start_res->status);
        break;
    }

    case MDIF__RFS__RFS_MSG__MSG_STOP_RES: {
        printf("Decode STOP_RES\n");
        printf("    status=%u\n", rfs_msg->stop_res->status);
        break;
    }

    case MDIF__RFS__RFS_MSG__MSG_GET_DRONE_INFO_RES: {
        printf("Decode DRONE INFO\n");
        // Reduce length of print outs
        Mdif__Rfs__DroneInfo *drone_info = rfs_msg->get_drone_info_res->drone_info;
        printf("     type_id          = %d\n", drone_info->type_id);
        printf("     type_id_name     = %s\n", drone_info->type_id_name);
        printf("     drone_name       = %s\n", drone_info->drone_name);
        printf("     vendor_name      = %s\n", drone_info->vendor_id_name);
        printf("     category         = %u\n", drone_info->category);
        printf("     bands            = ");
        for (int i = 0; i < drone_info->n_band; i++) {
            printf("%i", drone_info->band[i]);
            if (i + 1 < drone_info->n_band)
                printf(", ");
        }
        printf("\n");
        printf("---------Next type_id = %u\n", rfs_msg->get_drone_info_res->next_drone_type_id);
        break;
    }

    case MDIF__RFS__RFS_MSG__MSG_RFS_THREAT_IND: {
        printf("Decode RFS_THREAT_IND\n");

        // Various scalars and strings picked at random
        printf("    id=%u\n", rfs_msg->rfs_threat_ind->id);
        printf("    type_id=%d\n", rfs_msg->rfs_threat_ind->type_id);
        printf("    power=%f\n", rfs_msg->rfs_threat_ind->power);
        if (rfs_msg->rfs_threat_ind->relative_bearing->valid) {
            printf("    relative_bearing:\n");
            printf("        bearing=%f\n", rfs_msg->rfs_threat_ind->relative_bearing->bearing);
            printf("        var_bearing=%f\n", rfs_msg->rfs_threat_ind->relative_bearing->var_bearing);
        }
        printf("    last_seen=%lu.%u\n", rfs_msg->rfs_threat_ind->last_seen_ts->seconds, rfs_msg->rfs_threat_ind->last_seen_ts->nanos);
        // Todo: Add repeated current band to response and display
        // Todo: Keep paired with .proto message response.
        break;
    }
    case MDIF__RFS__RFS_MSG__MSG_WIFI_THREAT_IND: {
        printf("Decode WIFI_THREAT_IND\n");

        // Various scalars and strings picked at random
        printf("    id=%u\n", rfs_msg->wifi_threat_ind->id);
        printf("    type_id=%d\n", rfs_msg->wifi_threat_ind->type_id);
        printf("    power=%f\n", rfs_msg->wifi_threat_ind->power);
        printf("    channel=%d\n", rfs_msg->wifi_threat_ind->channel);
        printf("    max_addr=");
        for (int i = 0; i < 6; i++) {
            printf("%02X", rfs_msg->wifi_threat_ind->mac_adr.data[i]);
            if (i < 5) {
                printf(":");
            }
        }
        printf("\n");
        // Todo: Add repeated current band to response and display
        // Todo: Keep paired with .proto message response.
        break;
    }
    case MDIF__RFS__RFS_MSG__MSG_THREAT_STOPPED_IND: {
        printf("Decode THREAT_STOPPED_IND\n");
        printf("    id=%u\n", rfs_msg->threat_stopped_ind->id);
        break;
    }

    default:
        rtn = DECODE_ERR_OTHER;
    }

    mdif__rfs__rfs_msg__free_unpacked(rfs_msg, NULL);

    printf("\n");

    return rtn;
}
