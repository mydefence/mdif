#include "yahdlc.h"
#include <stdio.h>

// HDLC Control field bit positions
#define YAHDLC_CONTROL_S_OR_U_FRAME_BIT 0
#define YAHDLC_CONTROL_SEND_SEQ_NO_BIT 1
#define YAHDLC_CONTROL_S_FRAME_TYPE_BIT 2
#define YAHDLC_CONTROL_POLL_BIT 4
#define YAHDLC_CONTROL_RECV_SEQ_NO_BIT 5

// HDLC Control type definitions
#define YAHDLC_CONTROL_TYPE_RECEIVE_READY 0
#define YAHDLC_CONTROL_TYPE_RECEIVE_NOT_READY 1
#define YAHDLC_CONTROL_TYPE_REJECT 2
#define YAHDLC_CONTROL_TYPE_SELECTIVE_REJECT 3

// Supervisory frames
#define YAHDLC_SFRAME_MASK 0x0F
#define YAHDLC_SFRAME_RR 0x11  // receive ready aka. ACK
#define YAHDLC_SFRAME_REJ 0x19 // reject aka. NACK

// Unnumbered frames
#define YAHDLC_UFRAME_MASK 0xEF // Ignore P/F bit
#define YAHDLC_UFRAME_UI 0x13
#define YAHDLC_UFRAME_SABM 0x3F
#define YAHDLC_UFRAME_UA 0x73

void yahdlc_escape_value(char value, char *dest, int *dest_index)
{
    // Check and escape the value if needed
    if ((value == YAHDLC_FLAG_SEQUENCE) || (value == YAHDLC_CONTROL_ESCAPE)) {
        dest[(*dest_index)++] = YAHDLC_CONTROL_ESCAPE;
        value ^= 0x20;
    }

    // Add the value to the destination buffer and increment destination index
    dest[(*dest_index)++] = value;
}

yahdlc_control_t yahdlc_get_control_type(unsigned char control)
{
    yahdlc_control_t value = {0};

    // Check if the frame is a S-frame (or U-frame)
    if (control & (1 << YAHDLC_CONTROL_S_OR_U_FRAME_BIT)) {
        if ((control & YAHDLC_SFRAME_MASK) == (YAHDLC_SFRAME_RR & YAHDLC_SFRAME_MASK)) {
            value.frame = YAHDLC_FRAME_ACK;
            value.recv_seq_no = (control >> YAHDLC_CONTROL_RECV_SEQ_NO_BIT);
        } else if ((control & YAHDLC_SFRAME_MASK) == (YAHDLC_SFRAME_REJ & YAHDLC_SFRAME_MASK)) {
            value.frame = YAHDLC_FRAME_NACK;
            value.recv_seq_no = (control >> YAHDLC_CONTROL_RECV_SEQ_NO_BIT);
        } else if ((control & YAHDLC_UFRAME_MASK) == (YAHDLC_UFRAME_UI & YAHDLC_UFRAME_MASK)) {
            value.frame = YAHDLC_FRAME_UI;
        } else if ((control & YAHDLC_UFRAME_MASK) == (YAHDLC_UFRAME_SABM & YAHDLC_UFRAME_MASK)) {
            value.frame = YAHDLC_FRAME_SABM;
        } else if ((control & YAHDLC_UFRAME_MASK) == (YAHDLC_UFRAME_UA & YAHDLC_UFRAME_MASK)) {
            value.frame = YAHDLC_FRAME_UA;
        } else {
            value.frame = YAHDLC_FRAME_NOT_SUPPORTED;
        }
    } else {
        // It must be an I-frame so add the send sequence number
        value.frame = YAHDLC_FRAME_DATA;
        value.recv_seq_no = (control >> YAHDLC_CONTROL_RECV_SEQ_NO_BIT);
        value.send_seq_no = (control >> YAHDLC_CONTROL_SEND_SEQ_NO_BIT);
    }

    return value;
}

unsigned char yahdlc_frame_control_type(yahdlc_control_t *control)
{
    unsigned char value = 0;

    // For details see: https://en.wikipedia.org/wiki/High-Level_Data_Link_Control
    switch (control->frame) {
    case YAHDLC_FRAME_DATA:
        // Create the HDLC I-frame control byte with Poll bit set
        value |= (control->send_seq_no << YAHDLC_CONTROL_SEND_SEQ_NO_BIT);
        value |= (control->recv_seq_no << YAHDLC_CONTROL_RECV_SEQ_NO_BIT);
        value |= (1 << YAHDLC_CONTROL_POLL_BIT);
        break;
    case YAHDLC_FRAME_UI:
        // Create the HDLC I-frame control byte with Poll bit set
        value = YAHDLC_UFRAME_UI;
        break;
    case YAHDLC_FRAME_SABM:
        // Create the HDLC I-frame control byte with Poll bit set
        value = YAHDLC_UFRAME_SABM;
        break;
    case YAHDLC_FRAME_UA:
        // Create the HDLC I-frame control byte with Poll bit set
        value = YAHDLC_UFRAME_UA;
        break;
    case YAHDLC_FRAME_ACK:
        // Create the HDLC Receive Ready S-frame control byte with Poll bit cleared
        value |= (control->recv_seq_no << YAHDLC_CONTROL_RECV_SEQ_NO_BIT);
        value |= (1 << YAHDLC_CONTROL_S_OR_U_FRAME_BIT);
        break;
    case YAHDLC_FRAME_NACK:
        // Create the HDLC Receive Ready S-frame control byte with Poll bit cleared
        value |= (control->recv_seq_no << YAHDLC_CONTROL_RECV_SEQ_NO_BIT);
        value |= (YAHDLC_CONTROL_TYPE_REJECT << YAHDLC_CONTROL_S_FRAME_TYPE_BIT);
        value |= (1 << YAHDLC_CONTROL_S_OR_U_FRAME_BIT);
        break;
    case YAHDLC_FRAME_NOT_SUPPORTED:
        // Cannot happen, case needed to avoid compiler warning
        break;
    }

    return value;
}

void yahdlc_get_data_reset_with_state(yahdlc_state_t *state)
{
    state->fcs = FCS_INIT_VALUE;
    state->start_index = state->end_index = -1;
    state->src_index = state->dest_index = 0;
    state->control_escape = 0;
}

int yahdlc_get_data_with_state(yahdlc_state_t *state, const char *src,
                               unsigned int src_len, char dest[YAHDLC_DEST_LEN], unsigned int *dest_len)
{
    int ret;
    char value;
    unsigned int i;

    // Make sure that all parameters are valid
    if (!state || !src || !dest || !dest_len) {
        return -EINVAL;
    }

    // Run through the data bytes
    for (i = 0; i < src_len; i++) {
        // First find the start flag sequence
        if (state->start_index < 0) {
            if (src[i] == YAHDLC_FLAG_SEQUENCE) {
                // Check if an additional flag sequence byte is present
                if ((i < (src_len - 1)) && (src[i + 1] == YAHDLC_FLAG_SEQUENCE)) {
                    // Just loop again to silently discard it (accordingly to HDLC)
                    continue;
                }

                state->start_index = state->src_index;
            }
        } else {
            // Check for end flag sequence
            if (src[i] == YAHDLC_FLAG_SEQUENCE) {
                // Check if an additional flag sequence byte is present or earlier received
                if (((i < (src_len - 1)) && (src[i + 1] == YAHDLC_FLAG_SEQUENCE)) || ((state->start_index + 1) == state->src_index)) {
                    // Just loop again to silently discard it (accordingly to HDLC)
                    continue;
                }

                state->end_index = state->src_index;
                break;
            } else if (src[i] == YAHDLC_CONTROL_ESCAPE) {
                state->control_escape = 1;
                continue;
            } else {
                // Update the value based on any control escape received
                if (state->control_escape) {
                    state->control_escape = 0;
                    value = src[i] ^ 0x20;
                } else {
                    value = src[i];
                }

                // Now update the FCS value
                state->fcs = calc_fcs(state->fcs, value);

                if (state->src_index == state->start_index + 2) {
                    // Control field is the second byte after the start flag sequence
                    state->control = yahdlc_get_control_type(value);
                } else if (state->src_index > (state->start_index + 2)) {
                    if (state->dest_index >= YAHDLC_DEST_LEN) {
                        // Return buffer overflow error and indicate that data
                        // up to end flag sequence in buffer should be discarded
                        yahdlc_get_data_reset_with_state(state);
                        *dest_len = i + 1;
                        return -EIO;
                    }
                    // Start adding the data values after the Control field to the buffer
                    dest[state->dest_index++] = value;
                }
            }
        }
        state->src_index++;
    }

    // Check for invalid frame (no start or end flag sequence)
    if ((state->start_index < 0) || (state->end_index < 0)) {
        // Return no message and make sure destination length is 0
        *dest_len = 0;
        ret = -ENOMSG;
    } else {
        // A frame is at least 4 bytes in size and has a valid FCS value
        if ((state->end_index < (state->start_index + 4)) || (state->fcs != FCS_GOOD_VALUE)) {
            // Return FCS error and indicate that data up to end flag sequence in buffer should be discarded
            *dest_len = i;
            ret = -EIO;
        } else {
            // Return success and indicate that data up to end flag sequence in buffer should be discarded
            *dest_len = state->dest_index - sizeof(state->fcs);
            ret = i;
        }

        // Reset values for next frame
        yahdlc_get_data_reset_with_state(state);
    }

    return ret;
}

int yahdlc_frame_data(yahdlc_control_t *control, const char *src,
                      unsigned int src_len, char dest[YAHDLC_MAX_ENCODED_LEN], unsigned int *dest_len)
{
    unsigned int i;
    int dest_index = 0;
    unsigned char value = 0;
    FCS_SIZE fcs = FCS_INIT_VALUE;

    // Make sure that all parameters are valid
    if (!control || (!src && (src_len > 0)) || !dest || !dest_len) {
        return -EINVAL;
    }

    // Start by adding the start flag sequence
    dest[dest_index++] = YAHDLC_FLAG_SEQUENCE;

    // Add the all-station address from HDLC (broadcast)
    fcs = calc_fcs(fcs, YAHDLC_ALL_STATION_ADDR);
    yahdlc_escape_value(YAHDLC_ALL_STATION_ADDR, dest, &dest_index);

    // Add the framed control field value
    value = yahdlc_frame_control_type(control);
    fcs = calc_fcs(fcs, value);
    yahdlc_escape_value(value, dest, &dest_index);

    // Only DATA frames should contain data
    if (control->frame == YAHDLC_FRAME_DATA || control->frame == YAHDLC_FRAME_UI) {
        // Calculate FCS and escape data
        for (i = 0; i < src_len; i++) {
            fcs = calc_fcs(fcs, src[i]);
            yahdlc_escape_value(src[i], dest, &dest_index);
        }
    }

    // Invert the FCS value accordingly to the specification
    fcs ^= FCS_INVERT_MASK;

    // Run through the FCS bytes and escape the values
    for (i = 0; i < sizeof(fcs); i++) {
        value = ((fcs >> (8 * i)) & 0xFF);
        yahdlc_escape_value(value, dest, &dest_index);
    }

    // Add end flag sequence and update length of frame
    dest[dest_index++] = YAHDLC_FLAG_SEQUENCE;
    *dest_len = dest_index;

    return 0;
}
