/// Broadly applicable state and storage management module header.
/**
 ** \file badge.h
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#ifndef BADGE_H_
#define BADGE_H_

uint8_t is_handler(uint16_t id);
uint8_t is_uber(uint16_t id);

void set_badge_seen(uint16_t id, uint8_t *name);

void init_config();

#endif /* BADGE_H_ */
