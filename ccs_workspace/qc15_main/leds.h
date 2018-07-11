/*
 * leds.h
 *
 *  Created on: Jul 9, 2018
 *      Author: george
 */

#ifndef LEDS_H_
#define LEDS_H_

#include <stdint.h>
#include "ht16d35b.h"

// LED animation states:
#define LED_ANIM_TYPE_NONE 0
#define LED_ANIM_TYPE_SPIN 1
#define LED_ANIM_TYPE_SAME 2
#define LED_ANIM_TYPE_FALL 3

#define DEFAULT_ANIM_SPEED 30

typedef struct {
    int_fast16_t r;
    int_fast16_t g;
    int_fast16_t b;
} rgbdelta_t;

typedef struct {
    const rgbcolor_t * colors;
    uint8_t len;
    uint8_t speed; // csecs per frame
    char anim_name[12];
} led_ring_animation_t;

extern uint8_t s_led_anim_done;

void led_timestep();
void led_set_anim(led_ring_animation_t *anim, uint8_t anim_type, uint8_t loops, uint8_t use_pad_in_loops);

#endif /* LEDS_H_ */
