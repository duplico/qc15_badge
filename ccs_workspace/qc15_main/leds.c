/*
 * leds.c
 *
 *  Created on: Jul 9, 2018
 *      Author: george
 */

#include <stdint.h>
#include <string.h>

#include "qc15.h"
#include "leds.h"
//#include "led_animations.h" // TODO

uint8_t led_anim_type = 0;
uint16_t led_ring_anim_step = 0;
uint8_t led_ring_anim_index = 0;
led_ring_animation_t led_ring_anim_curr;
rgbcolor_t led_ring_curr[18];
rgbcolor_t led_ring_dest[18];
rgbdelta_t led_ring_step[18];

uint8_t led_line_state = 0;
//rgbcolor_t led_line_curr[6];
//rgbcolor_t led_line_dest[6];
//rgbdelta_t led_line_step[6];

void led_init() {
    memset(led_ring_curr, 0, 18);
    memset(led_ring_dest, 0, 18);
    memset(led_ring_step, 0, 18);

//    memset(led_line_curr, 0, 6);
//    memset(led_line_dest, 0, 6);
//    memset(led_line_step, 0, 6);
}

void led_set_anim(led_ring_animation_t anim) {
    led_ring_anim_curr = anim;
    led_ring_anim_step = 0;
    led_ring_anim_index = 0;
}

/// LED timestep function, which should be called approx. 100x per second.
void led_timestep() {
    if (!led_anim_type) {
        // LED_ANIM_TYPE_NONE
        return;
    }

    led_ring_anim_step++;
    if (led_ring_anim_step >= led_ring_anim_curr.speed) {
        // fade is complete.
    } else {
        for (uint8_t i=0; i<18; i++) {
            led_ring_curr[i].r+= led_ring_step[i].r;
            led_ring_curr[i].g+= led_ring_step[i].g;
            led_ring_curr[i].b+= led_ring_step[i].b;
        }
    }


}
