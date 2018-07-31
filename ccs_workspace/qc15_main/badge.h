/// Broadly applicable state and storage management module header.
/**
 ** \file badge.h
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#ifndef BADGE_H_
#define BADGE_H_

// Persistent values:
extern qc15conf badge_conf;
extern qc15conf backup_conf;
extern uint8_t badge_names[QC15_BADGES_IN_SYSTEM][QC15_BADGE_NAME_LEN];
extern uint8_t person_names[QC15_BADGES_IN_SYSTEM][QC15_PERSON_NAME_LEN];
extern uint16_t stored_state_id;
extern uint16_t last_state_id;
extern uint16_t current_state_id;

uint8_t is_handler(uint16_t id);
uint8_t is_uber(uint16_t id);

uint8_t set_badge_seen(uint16_t id, uint8_t *name);
uint8_t set_badge_uploaded(uint16_t id);
uint8_t set_badge_downloaded(uint16_t id);
void load_person_name(uint8_t *buf, uint16_t id);
void load_badge_name(uint8_t *buf, uint16_t id);
void init_config();
void draw_text(uint8_t lcd_id, char *txt, uint8_t more);

extern uint8_t s_gd_success;
extern uint8_t s_gd_failure;

#endif /* BADGE_H_ */
