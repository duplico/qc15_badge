/*
 * radio.c
 *
 *  Created on: Jun 27, 2018
 *      Author: george
 */
#include <stdint.h>

#include <msp430.h>

#include "qc15.h"

#include "radio.h"
#include "rfm75.h"
#include "util.h"
#include "ipc.h"

/// An **insanely wasteful** array of all badges and bases we can currently see.
uint_least8_t ids_in_range[QC15_HOSTS_IN_SYSTEM] = {0};
/// The current radio packet we're sending (or just sent).
radio_proto curr_packet_tx;

uint8_t progress_tx_id = 0;

void radio_send_progress_frame(uint8_t frame_id) {
    radio_progress_payload *payload = (radio_progress_payload *)
                                            (curr_packet_tx.msg_payload);

    curr_packet_tx.badge_id = badge_status.badge_id;
    curr_packet_tx.msg_type = RADIO_MSG_TYPE_PROGRESS;
    curr_packet_tx.proto_version = RADIO_PROTO_VER;

    payload->part_id = badge_status.code_starting_part + frame_id;
    memcpy(payload->part_data, badge_status.code_part_unlocks[frame_id],
           CODE_SEGMENT_REP_LEN);
    crc16_append_buffer(&curr_packet_tx, sizeof(radio_proto)-2);

    rfm75_tx(RFM75_BROADCAST_ADDR, 1, &curr_packet_tx, RFM75_PAYLOAD_SIZE);
}

uint8_t validate(radio_proto *msg, uint8_t len) {
    if (len != sizeof(radio_proto)) {
        // PROBLEM
        // TODO: check that this is working correctly, please.
        return 0;
    }

    // Check for bad ID:
    if (msg->badge_id >= QC15_HOSTS_IN_SYSTEM)
        return 0;

    // Finally, verify the CRC:
    return (crc16_check_buffer((uint8_t *) msg, len-2));
}

void radio_handle_beacon(uint16_t id, radio_beacon_payload *payload) {
    // construct an alert to the main badge:

    if (id < QC15_BADGES_IN_SYSTEM) {
        // It's a badge.
        ipc_msg_gd_arr_t ipc_out;
        ipc_out.badge_id = id;
        memcpy(ipc_out.name, payload->name, QC15_PERSON_NAME_LEN);

        if (ids_in_range[id]) {
            // Badge was already in range, so now just re-up its flush counter.
            ids_in_range[id] = RADIO_GD_INTERVAL;
        } else if (ipc_tx_op_buf(IPC_MSG_GD_ARR, &ipc_out,
                                 sizeof(ipc_msg_gd_arr_t))) {
            // If it WASN'T already in range, we need to signal that it is now
            //  in range. But it's very important that this message actually get
            //  to the main MCU. So we're going to IGNORE the badge if our link
            //  to the main MCU isn't available, and only treat it as in range
            //  if we're OK to use the link. It's not very likely that it's
            //  unavailable, but we still want to avoid having an inconsistency
            //  between the two processors.
            // Then again, a foolish consistency is the hobgoblin of little
            //  minds, adored by little statemen and philosophers and divines.
            //  So what the heck do I know?
            ids_in_range[id] = RADIO_GD_INTERVAL;
        }
    } else if (id == QC15_BASE_ID) {
        // It's the suite base
        // Let's transmit our progress!
        progress_tx_id = 0;
        radio_send_progress_frame(0);
    } else if (id == QC15_CONTROL_ID) {
        // It's the event controller
    }

    // That was easy. The main MCU will handle the rest of the logic. All we
    //  care about is keeping track of who's in range.

    // TODO: Handle clock setting
    // TODO: if our crystal is broken, tend to accept updates from other
    //  badges.

}

/// Another badge has connected to us and DOWNLOADED OUR INFORMATION BRAIN.
void radio_handle_download(uint16_t id, radio_connect_payload *payload) {
    // This while loop is to KEEP TRYING TO TRANSMIT until we are successful.
    //  It's hideously inefficient and should be done asynchronously, instead.
    //  But for the nonce, this is the way it is. Hopefully we'll have time to
    //  change it, but if not, for consistency, it's very important that this
    //  message make it across, because we've already ACKed it, for goodness
    //  sake.
    // TODO: this is dumb:
    // TODO: Do we need to update the name? I submit that we likely do not.
    while (!ipc_tx_op_buf(IPC_MSG_GD_UL, &id, 2));
}

void radio_rx_done(uint8_t* data, uint8_t len, uint8_t pipe) {
    radio_proto *msg = (radio_proto *)data;
    if (!validate(msg, len)) {
        // fail
        return;
    }

    switch(msg->msg_type) {
    case RADIO_MSG_TYPE_BEACON:
        // Handle a beacon.
        radio_handle_beacon(msg->badge_id,
                            (radio_beacon_payload *) (msg->msg_payload));
        break;
    case RADIO_MSG_TYPE_DLOAD:
        // Handle a direct connection to DOWNLOAD OUR INFORMATION BRAINS
        radio_handle_download(msg->badge_id,
                              (radio_connect_payload *) (msg->msg_payload));
    }
}

/// Called when the transmission of `curr_packet` has either finished or failed.
void radio_tx_done(uint8_t ack) {
    switch(curr_packet_tx.msg_type) {
        case RADIO_MSG_TYPE_BEACON:
            // We just sent a beacon.
            // TODO: Clear any state that needs cleared.
            break;
        case RADIO_MSG_TYPE_DLOAD:
            // We just attempted a download. Did it succeed?
            if (ack) {
                // yes.
            } else {
                // no.
            }
            break;
        case RADIO_MSG_TYPE_PROGRESS:
            // We just sent a progress message. Determine whether we need
            //  to send another, or whether that was the last one.
            if (progress_tx_id == 5) {
                // This concludes our progress messages.
                break;
            }
            // These are unicast messages subject to auto-acknowledgment, but
            //  currently we're not bothering to check the result and resend;
            //  we're just using the auto-ack mechanism to try to add a bit
            //  more reliability.
            // If we DO need to send another one, we can just do it from here,
            //  rather than setting a flag; this is because we ARE allowed to
            //  call rfm75_tx() from inside the callback.
            progress_tx_id++;
            radio_send_progress_frame(progress_tx_id);
            break;
        case RADIO_MSG_TYPE_STATS:
            // We just sent our STATS somewhere.
            // I don't actually know whether we need to do anything after
            //  we've done this. In fact, I'm not sure whether we're even
            //  actually ever going to do it.
            break;
    }
}

/// Do our regular radio and gaydar interval actions.
/**
 * This function MUST NOT be called if we are in a state where the radio is
 * not allowed to initiate a transmission, because it will ALWAYS call
 * `rfm75_tx()`. That guard MUST be done outside of this function, because
 * this function has MANY side effects.
 */
void radio_interval() {
    for (uint16_t i=0; i<QC15_HOSTS_IN_SYSTEM; i++) {
        if (ids_in_range[i] == 1) {
            // Try sending a message to the main MCU that this badge has
            //  aged out. If it's successful, we can actually age it out.
            //  If not, we need to wait for the next interval and try again
            //  until we're not sending anymore.
            if (ipc_tx_op_buf(IPC_MSG_GD_DEP, &i, 2))
                ids_in_range[i] = 0;
        } else if (ids_in_range[i]) {
            // Otherwise, just decrement it.
            ids_in_range[i]--;
        }
    }

    // Also, at each radio interval, we do need to do a beacon.
    radio_beacon_payload *payload = (radio_beacon_payload *)
                                            (curr_packet_tx.msg_payload);
    curr_packet_tx.badge_id = badge_status.badge_id;
    curr_packet_tx.msg_type = RADIO_MSG_TYPE_BEACON;
    curr_packet_tx.proto_version = RADIO_PROTO_VER;
    payload->time = (qc_clock & 0x00FFFFFF); // Mask out the MSByte

    memcpy(payload->name, badge_status.person_name, QC15_PERSON_NAME_LEN);
    crc16_append_buffer(&curr_packet_tx, sizeof(radio_proto)-2);

    // Send our beacon.
    rfm75_tx(RFM75_BROADCAST_ADDR, 1, &curr_packet_tx, RFM75_PAYLOAD_SIZE);
}

void radio_init(uint16_t addr) {
    rfm75_init(addr, &radio_rx_done, &radio_tx_done);
    rfm75_post();
}
