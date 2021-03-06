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
badge_info_t ids_in_range[QC15_HOSTS_IN_SYSTEM] = {0};
/// The current radio packet we're sending (or just sent).
radio_proto curr_packet_tx;

uint8_t progress_tx_id = 0;
uint8_t s_need_progress_tx = 0;

uint16_t rx_cnt[FREQ_NUM] = {0,};

#pragma PERSISTENT(radio_frequency)
uint8_t radio_frequency = FREQ_MIN;
#pragma PERSISTENT(radio_frequency_done)
uint8_t radio_frequency_done = 0;

void radio_send_progress_frame(uint8_t frame_id) {
    radio_progress_payload *payload = (radio_progress_payload *)
                                            (curr_packet_tx.msg_payload);

    curr_packet_tx.badge_id = badge_status.badge_id;
    curr_packet_tx.msg_type = RADIO_MSG_TYPE_PROGRESS;
    curr_packet_tx.proto_version = RADIO_PROTO_VER;

    payload->part_id = badge_status.code_starting_part + frame_id;
    memcpy(payload->part_data, badge_status.code_part_unlocks[frame_id],
           CODE_SEGMENT_REP_LEN);
    crc16_append_buffer((uint8_t *)&curr_packet_tx, sizeof(radio_proto)-2);

    rfm75_tx(RFM75_BROADCAST_ADDR, 1, (uint8_t *)&curr_packet_tx,
             RFM75_PAYLOAD_SIZE);
}

void radio_send_status() {
    radio_stats_payload *payload = (radio_stats_payload *)
                                        (curr_packet_tx.msg_payload);

    curr_packet_tx.badge_id = badge_status.badge_id;
    curr_packet_tx.msg_type = RADIO_MSG_TYPE_STATS;
    curr_packet_tx.proto_version = RADIO_PROTO_VER;

    memcpy(payload, &badge_status.badges_seen_count, 12);

    crc16_append_buffer((uint8_t *)&curr_packet_tx, sizeof(radio_proto)-2);

    rfm75_tx(RFM75_BROADCAST_ADDR, 1, (uint8_t *)&curr_packet_tx,
             RFM75_PAYLOAD_SIZE);
}

uint8_t validate(radio_proto *msg, uint8_t len) {
    if (len != sizeof(radio_proto)) {
        // PROBLEM
        return 0;
    }

    // Check for bad ID:
    if (msg->badge_id >= QC15_HOSTS_IN_SYSTEM)
        return 0;

    // Finally, verify the CRC:
    return (crc16_check_buffer((uint8_t *) msg, len-2));
}

void set_badge_in_range(uint16_t id, uint8_t *name) {
    if (!ids_in_range[id].intervals_left) {
        // This badge is not currently in range.
        ipc_msg_gd_arr_t ipc_out;
        ipc_out.badge_id = id;
        memcpy(ipc_out.name, name, QC15_BADGE_NAME_LEN);
        // Guarantee send:
        while (!ipc_tx_op_buf(IPC_MSG_GD_ARR, (uint8_t *)&ipc_out,
                              sizeof(ipc_msg_gd_arr_t)));
        if (id == QC15_BASE_ID) {
            // It's the suite base
            // Let's transmit our progress!
            progress_tx_id = 0;
            s_need_progress_tx = 1;
        } else if (id == QC15_CONTROL_ID) {
            // It's the remote control thingy, which may or may not do anything.
        } else if (id >= QC15_EVENT_ID_START && id <= QC15_EVENT_ID_END) {
            // It's an event beacon.
            // Do we need to do anything special?
        }
    }
    ids_in_range[id].intervals_left = RADIO_GD_INTERVAL;
}

void radio_handle_beacon(uint16_t id, radio_beacon_payload *payload) {
    if (id < QC15_HOSTS_IN_SYSTEM) {
        // Whatever it is (badge, base, event, controller),
        //  inform the main MCU.
        set_badge_in_range(id, payload->name);
    }

    // That was easy. The main MCU will handle the rest of the logic. All we
    //  care about is keeping track of who's in range.

    // We ignore faulty incoming clocks.
    if (payload->time.fault)
        return;

    if (!qc_clock.authoritative || qc_clock.fault) {
        if (payload->time.authoritative || payload->time.time > qc_clock.time) {
            // We adjust our time if the remote clock authoritative,
            //  or if it's not authoritative but has been on longer.
            qc_clock.authoritative = payload->time.authoritative;
            qc_clock.time = payload->time.time;
        }
    }
}

/// Another badge has connected to us and DOWNLOADED OUR INFORMATION BRAIN.
void radio_handle_download(uint16_t id, radio_connect_payload *payload) {
    // There are two possibilities here. The first is that we've received a
    //  CONNECTABLE ADVERTISEMENT (RADIO_CONNECT_FLAG_LISTENING).
    // The second is that we've received a DOWNLOAD_CONNECTION
    //  (RADIO_CONNECT_FLAG_DOWNLOAD).

    if (payload->connect_flags == RADIO_CONNECT_FLAG_LISTENING) {
        // We need to mark this badge as connectable.
        ids_in_range[id].connect_intervals = 2;
        // We also treat this like a beacon, and update the name.
        set_badge_in_range(id, &payload->name[0]);
    } else if (payload->connect_flags == RADIO_CONNECT_FLAG_DOWNLOAD) {
        // Inform our main MCU that this badge has downloaded our information
        //  brain. (We don't actually track whether we're connectable - the
        //  client badge has to do that.)
        // This while loop is to KEEP TRYING TO TRANSMIT until we're successful.
        //  It's hideously inefficient and should be done asynchronously,
        //  but it's not. Deal with it <file://../../misc/dealwithit.gif>.
        while (!ipc_tx_op_buf(IPC_MSG_GD_UL, (uint8_t *)&id, 2));
    }
}

void radio_rx_done(uint8_t* data, uint8_t len, uint8_t pipe) {
    radio_proto *msg = (radio_proto *)data;

    if (!radio_frequency_done) {
        rx_cnt[radio_frequency - FREQ_MIN]++;
    }

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
    radio_connect_payload *rcp;
    switch(curr_packet_tx.msg_type) {
        case RADIO_MSG_TYPE_BEACON:
            // We just sent a beacon.
            // There's no state that needs to be cleared at this point.
            break;
        case RADIO_MSG_TYPE_DLOAD:
            // We just attempted a download. Did it succeed?
            rcp = (radio_connect_payload*) (curr_packet_tx.msg_payload);
            if (rcp->connect_flags == RADIO_CONNECT_FLAG_DOWNLOAD) {
                // REALLY, if our radios were reliable enough, we would do
                //  the following:
                  /*
                  if (ack) {
                      while (!ipc_tx_byte(IPC_MSG_GD_DL_SUCCESS));
                  } else {
                      // no.
                      while (!ipc_tx_byte(IPC_MSG_GD_DL_FAILURE));
                  }
                 */
                // HOWEVER, because we sometimes have radio issues, and
                //  because in the UI we display the readiness of a target
                //  to be connected to, it's bad UX to have it fail for
                //  seemingly no reason. For that reason, we accept the
                //  tradeoff that sometimes a connection may not be
                //  recorded as BOTH an upload and a download, and we do the
                //  SUCCESS transmission in the IPC handler, rather than here.
                // This if statement is left in the code in order to provide
                //  the opportunity to explain this logic - and to change it
                //  in the future if it's decided that it should be.
            }
            // If we just sent an advertisement, we don't care.
            break;
        case RADIO_MSG_TYPE_PROGRESS:
            // We just sent a progress message. Determine whether we need
            //  to send another, or whether that was the last one.
            if (progress_tx_id == 5) {
                // This concludes our progress messages.
                // Send a stats message.
                radio_send_status();
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
            //  we've done this. Probably not.
            // Break.
            break;
    }
}

void radio_set_connectable() {
    curr_packet_tx.badge_id = badge_status.badge_id;
    curr_packet_tx.msg_type = RADIO_MSG_TYPE_DLOAD;
    curr_packet_tx.proto_version = RADIO_PROTO_VER;

    radio_connect_payload *payload = (radio_connect_payload *)
                                            (curr_packet_tx.msg_payload);
    payload->connect_flags = RADIO_CONNECT_FLAG_LISTENING;
    memcpy(payload->name, badge_status.person_name, QC15_PERSON_NAME_LEN);

    crc16_append_buffer((uint8_t *)&curr_packet_tx, sizeof(radio_proto)-2);
    rfm75_tx(RFM75_BROADCAST_ADDR, 1, (uint8_t *)&curr_packet_tx,
             RFM75_PAYLOAD_SIZE);

}

void radio_send_download(uint16_t id) {
    curr_packet_tx.badge_id = badge_status.badge_id;
    curr_packet_tx.msg_type = RADIO_MSG_TYPE_DLOAD;
    curr_packet_tx.proto_version = RADIO_PROTO_VER;

    radio_connect_payload *payload = (radio_connect_payload *)
                                            (curr_packet_tx.msg_payload);
    payload->connect_flags = RADIO_CONNECT_FLAG_DOWNLOAD;
    memcpy(payload->name, badge_status.person_name, QC15_PERSON_NAME_LEN);

    crc16_append_buffer((uint8_t *)&curr_packet_tx, sizeof(radio_proto)-2);
    // Send a UNICAST! With ACKING.
    rfm75_tx(id, 0, (uint8_t *)&curr_packet_tx, RFM75_PAYLOAD_SIZE);
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
        if (ids_in_range[i].intervals_left == 1) {
            // Try sending a message to the main MCU that this badge has
            //  aged out. If it's successful, we can actually age it out.
            //  If not, we need to wait for the next interval and try again
            //  until we're not sending anymore.
            if (ipc_tx_op_buf(IPC_MSG_GD_DEP, (uint8_t *)&i, 2))
                ids_in_range[i].intervals_left = 0;
        } else if (ids_in_range[i].intervals_left) {
            // Otherwise, just decrement it.
            ids_in_range[i].intervals_left--;
        }

        // No signals needed for this one:
        if (ids_in_range[i].connect_intervals)
            ids_in_range[i].connect_intervals--;

    }

    // Also, at each radio interval, we do need to do a beacon.
    radio_beacon_payload *payload = (radio_beacon_payload *)
                                            (curr_packet_tx.msg_payload);
    curr_packet_tx.badge_id = badge_status.badge_id;
    curr_packet_tx.msg_type = RADIO_MSG_TYPE_BEACON;
    curr_packet_tx.proto_version = RADIO_PROTO_VER;

    qc_clock_t temp_clock;
    temp_clock.time = qc_clock.time;
    temp_clock.authoritative = qc_clock.authoritative;
    temp_clock.fault = qc_clock.fault;
    memcpy(&payload->time, (uint8_t *)&temp_clock, sizeof(qc_clock_t));

    memcpy(payload->name, badge_status.person_name, QC15_PERSON_NAME_LEN);
    crc16_append_buffer((uint8_t *)&curr_packet_tx, sizeof(radio_proto)-2);

    // Send our beacon.
    rfm75_tx(RFM75_BROADCAST_ADDR, 1, (uint8_t *)&curr_packet_tx,
             RFM75_PAYLOAD_SIZE);
}

void radio_event_beacon() {
    if (!badge_status.event_beacon)
        return;

    radio_beacon_payload *payload = (radio_beacon_payload *)
                                                        (curr_packet_tx.msg_payload);
    curr_packet_tx.badge_id = QC15_EVENT_ID_START + badge_status.event_id;
    curr_packet_tx.msg_type = RADIO_MSG_TYPE_BEACON;
    curr_packet_tx.proto_version = RADIO_PROTO_VER;

    qc_clock_t temp_clock;
    temp_clock.time = qc_clock.time;
    temp_clock.authoritative = qc_clock.authoritative;
    temp_clock.fault = qc_clock.fault;
    memcpy(&payload->time, (uint8_t *)&temp_clock, sizeof(qc_clock_t));

    memcpy(payload->name, badge_status.person_name, QC15_PERSON_NAME_LEN);
    crc16_append_buffer((uint8_t *)&curr_packet_tx, sizeof(radio_proto)-2);

    // Send our beacon.
    rfm75_tx(RFM75_BROADCAST_ADDR, 1, (uint8_t *)&curr_packet_tx,
             RFM75_PAYLOAD_SIZE);

}

void radio_init(uint16_t addr) {
    rfm75_init(addr, &radio_rx_done, &radio_tx_done);
    rfm75_post();
    rfm75_write_reg(0x05, radio_frequency);
}
