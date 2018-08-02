/// Broadly applicable state and storage management module header.
/**
 ** \file badge.h
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#ifndef BADGE_H_
#define BADGE_H_

#include "leds.h"

// Non-persistent:
extern uint8_t unlock_radio_status;

// Persistent values:
extern qc15conf badge_conf;
extern qc15conf backup_conf;
extern const char badge_names[QC15_BADGES_IN_SYSTEM][QC15_BADGE_NAME_LEN];
extern char person_names[QC15_BADGES_IN_SYSTEM][QC15_PERSON_NAME_LEN];
extern uint16_t stored_state_id;
extern uint16_t last_state_id;
extern uint16_t game_curr_state_id;
extern const led_ring_animation_t *led_ring_anim_bg;
extern uint8_t led_ring_anim_pad_loops_bg;
extern uint8_t led_anim_type_bg;

uint8_t is_handler(uint16_t id);
uint8_t is_uber(uint16_t id);

uint8_t set_badge_seen(uint16_t id, uint8_t *name);
uint8_t set_badge_uploaded(uint16_t id);
uint8_t set_badge_downloaded(uint16_t id);
void load_person_name(uint8_t *buf, uint16_t id);
void load_badge_name(uint8_t *buf, uint16_t id);
void init_config();
void draw_text(uint8_t lcd_id, char *txt, uint8_t more);
void qc15_set_mode(uint8_t mode);
uint8_t flag_unlocked(uint16_t flag_num);
void unlock_flag(uint16_t flag_num);


extern uint8_t s_gd_success;
extern uint8_t s_gd_failure;

#endif /* BADGE_H_ */
