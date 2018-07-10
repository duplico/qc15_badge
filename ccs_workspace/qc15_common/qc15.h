/*
 * qc15.h
 *
 *  Created on: Jul 7, 2018
 *      Author: george
 */

#ifndef QC15_H_
#define QC15_H_

typedef struct {
    uint16_t badge_id;
    uint32_t csecs_of_qc;
    uint8_t code_segment_ids[6];
    uint8_t code_segment_unlocks[6][10];
    uint8_t badges_seen[15];
    uint8_t badges_connected[15];
    uint32_t ubers_seen;
    uint32_t ubers_connected;
    uint32_t handlers_seen;
    uint32_t handlers_connected;
    uint16_t crc16;
} qc15conf;

#endif /* QC15_H_ */
