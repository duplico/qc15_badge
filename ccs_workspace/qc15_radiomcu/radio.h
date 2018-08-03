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

#define FREQ_MIN 10
#define FREQ_NUM 12

typedef struct {
    uint8_t connect_intervals : 2;
    uint8_t intervals_left : 4;
} badge_info_t;

typedef struct {
    uint16_t badge_id;
    uint8_t proto_version;
    uint8_t msg_type;
    uint8_t msg_payload[QC15_PERSON_NAME_LEN+5];
    uint16_t crc16;
} radio_proto;

typedef struct { // Beacon payload (broadcast)
    qc_clock_t time;
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
    uint16_t badges_downloaded_count;
    uint16_t badges_uploaded_count;
    uint8_t ubers_seen_count;
    uint8_t ubers_downloaded_count;
    uint8_t ubers_uploaded_count;
    uint8_t handlers_seen_count;
    uint8_t handlers_downloaded_count;
    uint8_t handlers_uploaded_count;
} radio_stats_payload;

extern radio_proto curr_packet_tx;
extern badge_info_t ids_in_range[QC15_HOSTS_IN_SYSTEM];
extern uint8_t progress_tx_id;
extern uint8_t s_need_progress_tx;

extern uint16_t rx_cnt[FREQ_NUM];
extern uint8_t radio_frequency;
extern uint8_t radio_frequency_done;

rfm75_rx_callback_fn radio_rx_done;
rfm75_tx_callback_fn radio_tx_done;
void radio_init(uint16_t addr);
void radio_interval();
void radio_event_beacon();
void radio_set_connectable();
void radio_send_download(uint16_t id);
void radio_send_progress_frame(uint8_t frame_id);

#endif /* RADIO_H_ */
