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

uint8_t led_ring_looping = 0;
uint8_t led_anim_type = 0;
uint16_t led_ring_anim_step = 0;
uint8_t led_ring_anim_index = 0;
led_ring_animation_t led_ring_anim_curr;
rgbcolor16_t led_ring_curr[18];
rgbcolor16_t led_ring_dest[18];
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

///// Place colors into a color frame element, accounting for padding.
///**
// ** This function is required because, due to fading between the colors in
// ** our animations, we must account for their beginnings and ends, in which
// ** we must either wrap (in the case of looping animations), or pad with
// ** something else (e.g. darkness) to avoid undefined colors.
// **
// */
//void led_stage_color(rgbcolor_t *dest_color_frame, uint8_t anim_index) {
//    uint8_t pad_len;
//    if (led_anim_type == TLC_ANIM_MODE_SAME) {
//        pad_len = 1;
//    } else if (led_anim_type == TLC_ANIM_MODE_SHIFT) {
//        pad_len = 18;
//    }
//    if (anim_index < pad_len || anim_index >= tlc_curr_anim_len - pad_len) {
//        // If the current index is in the pad, i.e. before or after the anim:
//        memcpy(dest_color_frame, &color_off, sizeof(rgbcolor_t));
//    } else {
//        // Current index is in the animation, not the pad:
//        memcpy(dest_color_frame, &tlc_curr_anim[anim_index-pad_len], sizeof(rgbcolor_t));
//    }
//}
//
/// Set up the current frame's color sets (i.e. dest and step).
void led_load_colors() {
    // Set led_ring_dest
    // Set led_ring_step

    // TODO: Demeter
    // TODO: Handle padding. This isn't ready for TLC_ANIM_MODE_SHIFT.

    if (led_anim_type == TLC_ANIM_MODE_SAME) {
        // TODO: Handle looping and padding.
        //       The last way I did that was like so:
//        // Stage in current color:
//        stage_color(&tlc_colors_curr[0], tlc_anim_index);
//        // Stage in next color:
//        stage_color(&tlc_colors_next[0], (tlc_anim_index+1) % tlc_curr_anim_len);
        led_ring_dest[0].r = led_ring_anim_curr.colors[(led_ring_anim_index+1) % led_ring_anim_curr.len].r << 7;
        led_ring_dest[0].g = led_ring_anim_curr.colors[(led_ring_anim_index+1) % led_ring_anim_curr.len].g << 7;
        led_ring_dest[0].b = led_ring_anim_curr.colors[(led_ring_anim_index+1) % led_ring_anim_curr.len].b << 7;

        led_ring_step[0].r = ((int_fast16_t) led_ring_dest[0].r - (int_fast16_t)led_ring_curr[0].r) / led_ring_anim_curr.speed;
        led_ring_step[0].g = ((int_fast16_t) led_ring_dest[0].g - (int_fast16_t)led_ring_curr[0].g) / led_ring_anim_curr.speed;
        led_ring_step[0].b = ((int_fast16_t) led_ring_dest[0].b - (int_fast16_t)led_ring_curr[0].b) / led_ring_anim_curr.speed;
    }
}

/// Send the colors to the underlying controller.
void led_set_colors() {
    led_all_one_color_ring_only(led_ring_curr[0].r >> 7,
                                led_ring_curr[0].g >> 7,
                                led_ring_curr[0].b >> 7);
}

/// Set the current LED ring animation.
void led_set_anim(led_ring_animation_t anim, uint8_t anim_type) {
    led_ring_anim_curr = anim;
    led_anim_type = anim_type;
    led_ring_anim_step = 0;
    led_ring_anim_index = 0;

    led_ring_curr[0].r = anim.colors[0].r  << 7;
    led_ring_curr[0].g = anim.colors[0].g  << 7;
    led_ring_curr[0].b = anim.colors[0].b  << 7;

    led_load_colors();

    led_set_colors();
}

/// LED timestep function, which should be called approx. 100x per second.
void led_timestep() {
    if (!led_anim_type) {
        // LED_ANIM_TYPE_NONE
        return;
    }

    led_ring_anim_step++;
    if (led_ring_anim_step >= led_ring_anim_curr.speed) {
        // fade is complete. Time for the destination.
        // TODO: put the following into a function:
        led_ring_curr[0].r = led_ring_dest[0].r;
        led_ring_curr[0].g = led_ring_dest[0].g;
        led_ring_curr[0].b = led_ring_dest[0].b;

        led_ring_anim_step = 0;
        led_ring_anim_index++;

        if (led_ring_anim_index == led_ring_anim_curr.len) {
            // animation is over.
            // TODO: Should we loop?
            led_ring_anim_index = 0;
        }

        // Stage the next color sets
        //  i.e., set curr, dest, and steps.
        led_load_colors();
        led_set_colors();

    } else {
        led_ring_curr[0].r+= led_ring_step[0].r;
        led_ring_curr[0].g+= led_ring_step[0].g;
        led_ring_curr[0].b+= led_ring_step[0].b;
//        // TODO: The following is a shitshow.
//        led_ring_curr[0].r = led_ring_anim_curr.colors[led_ring_anim_index].r + ((led_ring_dest[0].r - led_ring_anim_curr.colors[led_ring_anim_index].r) * led_ring_anim_step) / led_ring_anim_curr.speed;
//        led_ring_curr[0].g = led_ring_anim_curr.colors[led_ring_anim_index].g + ((led_ring_dest[0].g - led_ring_anim_curr.colors[led_ring_anim_index].g) * led_ring_anim_step) / led_ring_anim_curr.speed;
//        led_ring_curr[0].b = led_ring_anim_curr.colors[led_ring_anim_index].b + ((led_ring_dest[0].b - led_ring_anim_curr.colors[led_ring_anim_index].b) * led_ring_anim_step) / led_ring_anim_curr.speed;
        led_set_colors();

//        for (uint8_t i=0; i<((led_anim_type==TLC_ANIM_MODE_SHIFT)? 18 : 1); i++) {
//        }
    }


}
