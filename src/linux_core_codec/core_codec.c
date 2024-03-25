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
 * Program/file : core_codec.c                                                 *
 *                                                                             *
 * Description  :  This file is provided to help with 3rd party integration.   *
 *              :  It provides an example of MDIF Core encoding and decoding.  *
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

#include "core_codec.h"

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

/*******************************************************************************
 *                                 Implementation
 *******************************************************************************/

/*******************************************
 * Client sends encoded messages to device *
 *******************************************/

/**
 * Encodes a Core Get Device Info request message.
 *
 * This function constructs a Core Get Device Info request message and returns
 * the encoded message along with its size.
 *
 * @param size Pointer to a variable where the size of the encoded message will
 *             be stored.
 * @return A dynamically allocated uint8_t pointer containing the encoded
 *         message. The caller is responsible for freeing the memory when done.
 */
uint8_t *encode_core_get_device_info_req(uint32_t *size) {
    // Construct the payload
    Mdif__Core__GetDeviceInfoReq get_device_info_req = MDIF__CORE__GET_DEVICE_INFO_REQ__INIT;

    // Construct the message
    Mdif__Core__CoreMsg core_msg = MDIF__CORE__CORE_MSG__INIT;
    core_msg.msg_case            = MDIF__CORE__CORE_MSG__MSG_GET_DEVICE_INFO_REQ;
    core_msg.get_device_info_req = &get_device_info_req;

    // Memory allocation sizing
    *size        = mdif__core__core_msg__get_packed_size(&core_msg);
    uint8_t *msg = malloc(*size);
    mdif__core__core_msg__pack(&core_msg, msg);
    return msg;
}

/**
 * Encodes a Core Get Battery Status request message.
 *
 * This function constructs a Core Get Battery Status request message and
 * returns the encoded message along with its size.
 *
 * @param size Pointer to a variable where the size of the encoded message will
 *             be stored.
 * @return A dynamically allocated uint8_t pointer containing the encoded
 *         message. The caller is responsible for freeing the memory when done.
 */
uint8_t *encode_core_get_battery_status_req(uint32_t *size)
{
    // Construct the payload
    Mdif__Core__GetBatteryStatusReq get_battery_status_req = MDIF__CORE__GET_BATTERY_STATUS_REQ__INIT;

    // Construct the message
    Mdif__Core__CoreMsg core_msg    = MDIF__CORE__CORE_MSG__INIT;
    core_msg.msg_case               = MDIF__CORE__CORE_MSG__MSG_GET_BATTERY_STATUS_REQ;
    core_msg.get_battery_status_req = &get_battery_status_req;

    // Memory allocation sizing
    *size        = mdif__core__core_msg__get_packed_size(&core_msg);
    uint8_t *msg = malloc(*size);
    mdif__core__core_msg__pack(&core_msg, msg);
    return msg;
}

/**
 * Encodes a Core Reset request message.
 *
 * This function constructs a Core Reset request message and returns the encoded
 * message along with its size.
 *
 * @param size Pointer to a variable where the size of the encoded message will
 *             be stored.
 * @return A dynamically allocated uint8_t pointer containing the encoded
 *         message. The caller is responsible for freeing the memory when done.
 */
uint8_t *encode_core_reset_req(uint32_t *size)
{
    // Construct the payload
    Mdif__Core__ResetReq reset_req = MDIF__CORE__RESET_REQ__INIT;

    // Construct the message
    Mdif__Core__CoreMsg core_msg = MDIF__CORE__CORE_MSG__INIT;
    core_msg.msg_case            = MDIF__CORE__CORE_MSG__MSG_RESET_REQ;
    core_msg.reset_req           = &reset_req;

    // Memory allocation sizing
    *size        = mdif__core__core_msg__get_packed_size(&core_msg);
    uint8_t *msg = malloc(*size);
    mdif__core__core_msg__pack(&core_msg, msg);
    return msg;
}

/****************************************************
 * Client receives and decodes messages from device *
 ****************************************************/

/**
 * Decode a Core Message from a binary buffer.
 *
 * This function decodes a binary buffer containing a Core Message and processes
 * the decoded message based on its type. It prints information related to the
 * decoded message for debugging purposes and returns a decode result code.
 *
 * @param buf Pointer to the binary buffer containing the Core Message.
 * @param size Size of the binary buffer.
 * @return A decode result code indicating the outcome of the decoding process.
 *         Possible return values are DECODE_SUCCESS for successful decoding,
 *         DECODE_ERR_NO_DECODER if the message type is not set, and DECODE_ERR_OTHER
 *         for other decoding errors.
 */
decode_rtn_t decode_core(const uint8_t *buf, uint32_t size) {
    Mdif__Core__CoreMsg *core_msg = mdif__core__core_msg__unpack(NULL, size, buf);

    // Was unpack successful?
    if (core_msg == NULL) {
        printf("    ERROR payload doesn't decode\n\n");
        return DECODE_ERR_NO_DECODER;
    }

    // Default
    decode_rtn_t rtn = DECODE_SUCCESS;

    // Switch through options described in core.proto CoreMsg
    switch (core_msg->msg_case) {

    // Device does not have response to target.
    case MDIF__CORE__CORE_MSG__MSG__NOT_SET: {
        rtn = DECODE_ERR_NO_DECODER;
        break;
    }

    // Device responds to client
    case MDIF__CORE__CORE_MSG__MSG_PING_RES: {
        printf("Decode PING_RES\n");
        break;
    }

    case MDIF__CORE__CORE_MSG__MSG_RESET_RES: {
        printf("Decode RESET_RES\n");
        printf("    status=%d\n", core_msg->reset_res->status);
        break;
    }

    case MDIF__CORE__CORE_MSG__MSG_GET_DEVICE_INFO_RES: {
        printf("Decode GET_DEVICE_INFO_RES\n");
        printf("    device_type=%d\n", core_msg->get_device_info_res->device_type);
        printf("    device_version=%u\n", core_msg->get_device_info_res->device_version);
        printf("    serial_number=%s\n", core_msg->get_device_info_res->serial_number);
        printf("    sw_version=%s\n", core_msg->get_device_info_res->sw_version);
        break;
    }

    case MDIF__CORE__CORE_MSG__MSG_GET_BATTERY_STATUS_RES: {
        printf("Decode GET_BATTERY_RES\n");
        printf("    status=%d\n", core_msg->reset_res->status);
        if (core_msg->reset_res->status == MDIF__COMMON__STATUS__ERR_NOT_SUPPORTED) {
            printf("    No battery\n");
            break;
        } else if (core_msg->reset_res->status != MDIF__COMMON__STATUS__SUCCESS) {
            break;
        }
        printf("    bat_low: %s\n", core_msg->get_battery_status_res->bat_low ? "true" : "false");
        printf("    bat_charging: %s\n", core_msg->get_battery_status_res->bat_charging ? "true" : "false");
        printf("    bat_volt: %f\n", core_msg->get_battery_status_res->bat_volt);
        break;
    }

    default:
        printf("Core message not decoded: %u", core_msg->msg_case);
        rtn = DECODE_ERR_OTHER;
        break;
    }

    mdif__core__core_msg__free_unpacked(core_msg, NULL);

    printf("\n");

    return rtn;
}
