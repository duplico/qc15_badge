/// High-level module header to handle LED animations.
/**
 ** \file leds.h
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
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

#define DEFAULT_ANIM_SPEED 10

typedef struct {
    int_fast16_t r;
    int_fast16_t g;
    int_fast16_t b;
} rgbdelta_t;

typedef struct {
    const rgbcolor_t * colors;
    uint8_t len;
    uint8_t speed; // csecs per frame
    uint8_t brightness;
    uint8_t type;
    char name[17];
} led_ring_animation_t;

extern uint8_t s_led_anim_done;

void led_timestep();
void led_off();
void led_on();
void led_set_anim(const led_ring_animation_t *anim, uint8_t anim_type, uint8_t loops, uint8_t use_pad_in_loops);
void led_set_anim_none();
void led_activate_file_lights();

#endif /* LEDS_H_ */
