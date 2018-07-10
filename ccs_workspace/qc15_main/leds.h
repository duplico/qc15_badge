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
#define TLC_ANIM_MODE_IDLE  0
#define TLC_ANIM_MODE_SHIFT 1
#define TLC_ANIM_MODE_SAME  2

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

#endif /* LEDS_H_ */
