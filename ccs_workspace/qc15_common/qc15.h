/*
 * qc15.h
 *
 *  Created on: Jul 7, 2018
 *      Author: george
 */

#ifndef QC15_H_
#define QC15_H_

#define QC15_CRC_SEED 0x5321

typedef struct {
    uint16_t badge_id;
    uint8_t code_segment_ids[6];
    uint8_t code_segment_unlocks[6][10];
    uint16_t crc16;
} qc15status;

typedef struct {
    uint16_t badge_id;
    uint32_t csecs_of_qc; // TODO
    uint8_t code_segment_ids[6];
    uint8_t code_segment_unlocks[6][10];
    uint8_t badges_seen[57];
    uint8_t badges_downloaded[57];
    uint8_t badges_uploaded[57];
    uint32_t ubers_seen;
    uint32_t ubers_connected;
    uint32_t handlers_seen;
    uint32_t handlers_connected;
    uint16_t crc16;
} qc15conf;

// The following items are specific to the RADIO MCU:
#ifdef __MSP430FR2422__
#define CLOCK_FREQ_KHZ 1000
#endif

// The following are specific to the MAIN MCU:
#ifdef __MSP430FR5972__
#define CLOCK_FREQ_KHZ 8000
#endif


#endif /* QC15_H_ */
