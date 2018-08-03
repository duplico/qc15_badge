/*
 * led_animations.h
 *
 *  Created on: Jul 31, 2018
 *      Author: george
 */

#ifndef LED_ANIMATIONS_H_
#define LED_ANIMATIONS_H_

#include "state_definitions.h"

#define FLAG_COUNT 20

extern const led_ring_animation_t anim_rainbow_spin;
extern const led_ring_animation_t all_animations[GAME_ANIMS_LEN];

extern const led_ring_animation_t anim_countdown_tick;
extern const led_ring_animation_t anim_countdown_done;
extern const led_ring_animation_t anim_dl_done;

#endif /* LED_ANIMATIONS_H_ */
