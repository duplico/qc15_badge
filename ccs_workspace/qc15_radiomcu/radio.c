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

// ugggh, this is SO wasteful. TODO: use a linked list or SOMETHING:
uint8_t ids_in_range[QC15_HOSTS_IN_SYSTEM] = {0};

uint8_t validate(radio_proto *msg, uint8_t len) {
    if (len != sizeof(radio_proto)) {
        // PROBLEM
        // TODO: check that this is working correctly, please.
        return 0;
    }

    // Check for bad ID:
    if (msg->badge_id > QC15_BADGES_IN_SYSTEM)
        return 0;

    // Finally, verify the CRC:
    return (crc16_check_buffer((uint8_t *) msg, len-2));
}

void radio_handle_beacon(uint16_t id, radio_beacon_payload *payload) {
    // construct an alert to the main badge:
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
        //  to the main MCU isn't available, and only treat it as in range if
        //  we're OK to use the link. It's not very likely that it's
        //  unavailable, but we still want to avoid having an inconsistency
        //  between the two processors.
        // Then again, a foolish consistency is the hobgoblin of little minds,
        //  adored by little statemen and philosophers and divines. So what the
        //  heck do I know?
        ids_in_range[id] = RADIO_GD_INTERVAL;
        // TODO: If this is a base station:
        // Let's set a flag that says we need to transmit our STATS!
    }
    // That was easy. The main MCU will handle the rest of the logic. All we
    //  care about is keeping track of who's in range.
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

void radio_tx_done(uint8_t ack) {
    // TODO: flag if ack is false, so we can signal that a download has failed.
}

void radio_send_progress_frame(uint8_t frame_id) {
    radio_proto packet;
    radio_progress_payload *payload = (radio_progress_payload *) packet.msg_payload;

    packet.badge_id = badge_status.badge_id;
    packet.msg_type = RADIO_MSG_TYPE_PROGRESS;
    packet.proto_version = RADIO_PROTO_VER;

    payload->part_id = badge_status.code_segment_ids[frame_id];
    memcpy(payload->part_data, badge_status.code_segment_unlocks[frame_id],
           CODE_SEGMENT_REP_LEN);
    crc16_append_buffer(&packet, sizeof(radio_proto)-2);

    rfm75_tx(RFM75_BROADCAST_ADDR, 1, &packet, RFM75_PAYLOAD_SIZE);
}

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

    // TODO: Do we want this on the stack, or as a global?
    //  Probably, we don't want to do a freaking memcpy every time we call
    //  this function. So let's switch it to a global, I think.
    radio_proto packet;
    radio_beacon_payload *payload = (radio_beacon_payload *) packet.msg_payload;
    packet.badge_id = badge_status.badge_id;
    packet.msg_type = RADIO_MSG_TYPE_BEACON;
    packet.proto_version = RADIO_PROTO_VER;

//    payload->time = csecs_of_queercon; // TODO
    payload->time = 12345;
    memcpy(payload->name, badge_status.person_name, QC15_PERSON_NAME_LEN);
//    packet.crc16 = crc16_compute(&packet, sizeof(radio_proto)-2)
    crc16_append_buffer(&packet, sizeof(radio_proto)-2);

    rfm75_tx(RFM75_BROADCAST_ADDR, 1, &packet, RFM75_PAYLOAD_SIZE);
}

void radio_init() {
    rfm75_init(35, &radio_rx_done, &radio_tx_done);
    rfm75_post();
}
