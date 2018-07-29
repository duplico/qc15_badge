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
void load_person_name(uint8_t *buf, uint16_t id);
void load_badge_name(uint8_t *buf, uint16_t id);
void init_config();
void draw_text(uint8_t lcd_id, char *txt, uint8_t more);

extern uint8_t s_gd_success;
extern uint8_t s_gd_failure;

#endif /* BADGE_H_ */
