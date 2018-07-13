/*
 * qc15.h
 *
 *  Created on: Jul 7, 2018
 *      Author: george
 */

#ifndef QC15_H_
#define QC15_H_

#include "msp430.h"

#define QC15_CRC_SEED 0x5321

// TODO: we need to add 1 to all of these for the null term
#define QC15_PERSON_NAME_LEN 10
#define QC15_BADGE_NAME_LEN 10

// TODO:
// Number of radio flush intervals to keep a badge in the system:
#define RADIO_GD_INTERVAL 10

// NB: If a base ID higher than QC_BADGES_IN_SYSTEM is needed, then we will
//     also have to change our radio validator.
#define QC15_BADGES_IN_SYSTEM 450
#define QC15_BASE_ID QC15_BADGES_IN_SYSTEM
#define QC15_HOSTS_IN_SYSTEM 451

#define CODE_SEGMENT_REP_LEN 10

#define LPM_BITS LPM0_bits
#define LPM LPM0
#define LPM_EXIT LPM0_EXIT

// TODO: Single source of truth in this file, instead of this level of
//  duplication, please.
typedef struct {
    uint16_t badge_id;
    uint8_t person_name[QC15_PERSON_NAME_LEN];
    uint8_t code_segment_ids[6];
    uint8_t code_segment_unlocks[6][CODE_SEGMENT_REP_LEN];
    uint16_t badges_seen_count, badges_connected_count;
    uint8_t ubers_seen_count, ubers_connected_count;
    uint8_t handlers_seen_count, handlers_connected_count;
} qc15status;

typedef struct {
    uint16_t badge_id;
    uint32_t csecs_of_qc; // TODO
    uint8_t badges_seen[57];
    uint8_t badges_downloaded[57];
    uint8_t badges_uploaded[57];
    uint32_t ubers_seen;
    uint32_t ubers_connected;
    uint32_t handlers_seen;
    uint32_t handlers_connected;
    uint16_t crc16;
} qc15conf;

extern qc15status badge_status;

// The following items are specific to the RADIO MCU:
#ifdef __MSP430FR2422__
#define CLOCK_FREQ_KHZ 1000
#endif

// The following are specific to the MAIN MCU:
#ifdef __MSP430FR5972__
#define CLOCK_FREQ_KHZ 8000
extern qc15conf badge_conf;
#endif

void handle_ipc_rx(uint8_t *);

#endif /* QC15_H_ */
