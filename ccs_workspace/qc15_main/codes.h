/**
 *  \file codes.h
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#ifndef CODES_H_
#define CODES_H_

#include <stdint.h>

extern uint8_t my_segment_ids[6];

void decode_download(uint16_t downloaded_id);
void decode_upload();
void decode_event(uint8_t event_id);
uint8_t is_solved(uint8_t code_id);

#endif /* CODES_H_ */
