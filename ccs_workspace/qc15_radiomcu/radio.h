/*
 * radio.h
 *
 *  Created on: Jun 27, 2018
 *      Author: george
 */

#ifndef RADIO_H_
#define RADIO_H_

#include <stdint.h>
#include "rfm75.h"
#include "qc15.h"

#define RADIO_MSG_TYPE_BEACON 1
#define RADIO_MSG_TYPE_DLOAD 2
#define RADIO_MSG_TYPE_PROGRESS 3
#define RADIO_MSG_TYPE_STATS 4

#define RADIO_PROTO_VER 1
#define RADIO_CONNECT_ADVERTISEMENT_COUNT 3

#define RADIO_CONNECT_FLAG_LISTENING 1
#define RADIO_CONNECT_FLAG_DOWNLOAD 2

typedef struct {
    uint16_t badge_id;
    uint8_t proto_version;
    uint8_t msg_type;
    uint8_t msg_payload[QC15_PERSON_NAME_LEN+5];
    uint16_t crc16;
} radio_proto;

typedef struct { // Beacon payload (broadcast)
    /// 24 LSBits are the clock, and MSBit is `1` if the clock has authority.
    uint32_t time;
    uint8_t name[QC15_PERSON_NAME_LEN];
} radio_beacon_payload;

typedef struct { // Connect payload (unicast)
    uint8_t name[QC15_PERSON_NAME_LEN];
    uint8_t connect_flags;
} radio_connect_payload;

typedef struct { // Progress report payload (unicast to base)
    uint8_t part_id;
    uint8_t part_data[10]; // 80 bits per segment
} radio_progress_payload;

typedef struct { // Stats report payload (unicast to base???)
    uint16_t badges_seen_count;
    uint16_t badges_connected_count;
    uint16_t badges_uploaded_count;
    uint8_t ubers_seen_count;
    uint8_t ubers_connected_count;
    uint8_t ubers_uploaded_count;
    uint8_t handlers_seen;
    uint8_t handlers_connected;
    uint8_t handlers_uploaded_count;
} radio_stats_payload;

extern radio_proto curr_packet_tx;
extern uint_least8_t ids_in_range[QC15_HOSTS_IN_SYSTEM];

rfm75_rx_callback_fn radio_rx_done;
rfm75_tx_callback_fn radio_tx_done;
void radio_init(uint16_t addr);
void radio_interval();
void radio_set_connectable();

#endif /* RADIO_H_ */
