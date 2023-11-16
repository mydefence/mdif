/**
 * @file yahdlc.h
 */

#ifndef YAHDLC_H
#define YAHDLC_H

#include "fcs.h"
#include <errno.h>

/** HDLC start/end flag sequence */
#define YAHDLC_FLAG_SEQUENCE 0x7E

/** HDLC control escape value */
#define YAHDLC_CONTROL_ESCAPE 0x7D

/** HDLC all station address */
#define YAHDLC_ALL_STATION_ADDR 0xFF

/** Maximum length of HDLC user data. */
#ifndef YAHDLC_MAX_FRAME_LEN
#define YAHDLC_MAX_FRAME_LEN 2000
#endif

/** `dest` buffer worst case size. 2 bytes longer than maximum frame length,
 * because yahdlc internally needs 2 bytes for checksum handling. */
#define YAHDLC_DEST_LEN ((YAHDLC_MAX_FRAME_LEN) + 2)

/** Worst case length of HDLC encoded data. Happens if all data, control, and
 * checksum must be escaped. Probably worst case is actually a few bytes less.
 * [flag, address*, control*, data*, fcs*, flag]
 */
#define YAHDLC_MAX_ENCODED_LEN (6 + YAHDLC_MAX_FRAME_LEN * 2 + 2 * sizeof(FCS_SIZE))

/** Supported HDLC frame types */
typedef enum {
    YAHDLC_FRAME_DATA,
    YAHDLC_FRAME_ACK,
    YAHDLC_FRAME_NACK,
    YAHDLC_FRAME_UI,            // Unnumbered Information
    YAHDLC_FRAME_SABM,          // Set Asynchronous Balanced Mode. Used for link reset
    YAHDLC_FRAME_UA,            // Unnumbered Acknowledgement
    YAHDLC_FRAME_NOT_SUPPORTED, // Anything else received
} yahdlc_frame_t;

/** Control field information */
typedef struct {
    yahdlc_frame_t frame;
    unsigned char send_seq_no : 3; // a.k.a N(S) only used with YAHDLC_FRAME_DATA
    unsigned char recv_seq_no : 3; // a.k.a N(R), used in all frames
} yahdlc_control_t;

/** Variables used in yahdlc_get_data_with_state
 * to keep track of received buffers
 */
typedef struct {
    char control_escape;
    FCS_SIZE fcs;
    int start_index;
    int end_index;
    int src_index;
    int dest_index;
    yahdlc_control_t control;
} yahdlc_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieves data from specified buffer containing the HDLC frame. Frames can be
 * parsed from multiple buffers e.g. when received via UART.
 *
 * @param[inout] state State structure wich tracks state between calls to the function
 * @param[in] src Source buffer with frame
 * @param[in] src_len Source buffer length
 * @param[out] dest Destination buffer. Should be able to contain max frame size + 2 (for fcs).
 * @param[out] dest_len Destination buffer length
 * @retval >=0 Success (size of returned value should be discarded from source buffer)
 * @retval -EINVAL Invalid parameter
 * @retval -ENOMSG Invalid message
 * @retval -EIO Invalid FCS (size of dest_len should be discarded from source buffer)
 */
int yahdlc_get_data_with_state(yahdlc_state_t *state, const char *src,
                               unsigned int src_len, char dest[YAHDLC_DEST_LEN], unsigned int *dest_len);

/**
 * Resets state values that are under the pointer provided as argument

 * This function needs to be called before the first call to yahdlc_get_data_with_state
 *
 * @param[inout] state State structure wich tracks state between calls to yahdlc_get_data_with_state
 *
 */
void yahdlc_get_data_reset_with_state(yahdlc_state_t *state);

/**
 * Creates HDLC frame with specified data buffer.
 *
 * @param[in] control Control field structure with frame type and sequence number
 * @param[in] src Source buffer with data
 * @param[in] src_len Source buffer length
 * @param[out] dest Destination buffer (should be bigger than source buffer)
 * @param[out] dest_len Destination buffer length
 * @retval 0 Success
 * @retval -EINVAL Invalid parameter
 */
int yahdlc_frame_data(yahdlc_control_t *control, const char *src,
                      unsigned int src_len, char dest[YAHDLC_MAX_ENCODED_LEN], unsigned int *dest_len);

#ifdef __cplusplus
}
#endif

#endif
