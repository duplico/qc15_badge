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

uint8_t s_led_anim_done = 0;

uint8_t led_ring_looping = 0;
uint8_t led_anim_type = 0;
uint16_t led_ring_anim_step = 0;
uint8_t led_ring_anim_index = 0;
uint8_t led_ring_anim_loops = 0;
uint8_t led_ring_anim_pad_len = 0;
uint8_t led_ring_anim_len_padded = 0;
led_ring_animation_t *led_ring_anim_curr;
rgbcolor16_t led_ring_curr[18];
rgbcolor16_t led_ring_dest[18];
rgbdelta_t led_ring_step[18];

uint8_t led_line_state = 0;

void led_init() {
    memset(led_ring_curr, 0, sizeof(led_ring_curr));
    memset(led_ring_dest, 0, sizeof(led_ring_dest));
    memset(led_ring_step, 0, sizeof(led_ring_step));
}

const rgbcolor16_t color_off = {0, 0, 0};

/// Compute the index of the next frame in the current animation.
uint8_t next_anim_index(uint8_t index) {
    // TODO:
    return (index+1) % led_ring_anim_len_padded;

    // If we're currently looping, and the next index will be after the end
    //  of the animation section,
    if (led_ring_anim_loops &&
        (index+1 >= led_ring_anim_curr->len + led_ring_anim_pad_len)) {
        // then the next index will be the beginning of the animation
        //  section, which is just the length of the pad.
        return led_ring_anim_pad_len + (index + 1) % led_ring_anim_curr->len;
    } else {
        // If we're not looping, or at the end of the animation, then things
        //  are much easier. The next index is just the next index.
        return index+1;
    }

}

/// Place colors into a color frame element, accounting for padding.
/**
 ** Using an animation frame index, which indexes a length padded by the
 ** number of LEDs involved in the animation, determine:
 ** (1) whether `anim_index` is at a padding frame BEFORE the beginning, or
 **     AFTER the end of the animation (in which case the correct color
 **     for this LED is OFF), or
 ** (2) whether `anim_index` is in an actual animation frame, in which case
 **     the color for the destination LED is based on the data in our
 **     animation struct.
 **
 ** Then, after that determination is made, copy the correct color frame into
 **  the destination rgbcolor16_t of the provided pointer, which is expected
 **  to be an element of either led_ring_curr or led_ring_dest.
 **
 */
void led_stage_color(rgbcolor16_t *dest_color_frame, uint8_t anim_index) {
    uint8_t unpadded_index = 0;

    if (anim_index < led_ring_anim_pad_len ||
            anim_index >= led_ring_anim_curr->len + led_ring_anim_pad_len) {
        // If the current index is in the pad, i.e. before or after the anim,
        //  then turn the LED off until it's out of the pad.
        memcpy(dest_color_frame, &color_off, sizeof(rgbcolor16_t));
    } else {
        // Current index is in the animation, not the pad:
        unpadded_index = anim_index-led_ring_anim_pad_len;
        dest_color_frame->r = led_ring_anim_curr->colors[unpadded_index].r << 7;
        dest_color_frame->g = led_ring_anim_curr->colors[unpadded_index].g << 7;
        dest_color_frame->b = led_ring_anim_curr->colors[unpadded_index].b << 7;
    }
}

// Relocate the following function to RAM as it has multiple division
//  and modulo operations and will be executed frequently:
#pragma CODE_SECTION(led_load_colors,".run_from_ram")

/// Set up the current frame's color sets (i.e. dest and step).
void led_load_colors() {
    uint8_t num_colors;

    switch(led_anim_type) {
    case LED_ANIM_TYPE_SAME:
        num_colors = 1;
        break;
    case LED_ANIM_TYPE_SPIN:
        num_colors = 18;
        break;
    case LED_ANIM_TYPE_FALL:
        num_colors = 9;
        break;
    }

    for (uint8_t i=0; i<num_colors; i++) {
        led_stage_color(&led_ring_dest[i],
                        next_anim_index(led_ring_anim_index+i));

        led_ring_step[i].r = ((int_fast16_t) led_ring_dest[i].r - (int_fast16_t)led_ring_curr[i].r) / led_ring_anim_curr->speed;
        led_ring_step[i].g = ((int_fast16_t) led_ring_dest[i].g - (int_fast16_t)led_ring_curr[i].g) / led_ring_anim_curr->speed;
        led_ring_step[i].b = ((int_fast16_t) led_ring_dest[i].b - (int_fast16_t)led_ring_curr[i].b) / led_ring_anim_curr->speed;
    }
}

/// Send the colors to the underlying controller.
void led_display_colors() {
    if (led_anim_type == LED_ANIM_TYPE_SAME) {
        led_all_one_color_ring_only(led_ring_curr[0].r >> 7,
                                    led_ring_curr[0].g >> 7,
                                    led_ring_curr[0].b >> 7);
    } else if (led_anim_type == LED_ANIM_TYPE_SPIN) {
        ht16d_set_colors(0, 18, led_ring_curr);
    } else if (led_anim_type == LED_ANIM_TYPE_FALL) {
        for (uint8_t i=0; i<9; i++) {
            memcpy(&led_ring_curr[17-i], &led_ring_curr[i], sizeof(rgbcolor16_t));
        }
        ht16d_set_colors(0, 18, led_ring_curr);
    }
}

// TODO: We need to fix the padding at the END of the animation.
//       Namely, we need to do the WHOLE pad at the end.

/// Set the current LED ring animation.
void led_set_anim(led_ring_animation_t *anim, uint8_t anim_type, uint8_t loops) {
    led_ring_anim_curr = anim;
    led_anim_type = anim_type;
    led_ring_anim_step = 0;
    led_ring_anim_index = 0;
    led_ring_anim_loops = loops;

    if (anim_type == LED_ANIM_TYPE_SAME) {
        led_ring_anim_pad_len = 1;
    } else if (anim_type == LED_ANIM_TYPE_SPIN) {
        led_ring_anim_pad_len = 18;
    } else if (anim_type == LED_ANIM_TYPE_FALL) {
        led_ring_anim_pad_len = 9;
    }

    // We need the full-sized pad on either side of the animation.
    //  In the event of looping animations, we will not use the pad
    //  between loops.
    led_ring_anim_len_padded = anim->len + 2*led_ring_anim_pad_len;
    uint8_t num_colors;

    switch(led_anim_type) {
    case LED_ANIM_TYPE_SAME:
        num_colors = 1;
        break;
    case LED_ANIM_TYPE_SPIN:
        num_colors = 18;
        break;
    case LED_ANIM_TYPE_FALL:
        num_colors = 9;
        break;
    }

    for (uint8_t i=0; i<num_colors; i++) {
        led_stage_color(&led_ring_curr[i], i);
    }

    led_load_colors();
    led_display_colors();
}

// TODO: Change this rate, and give it a parameter for csecs elapsed.
/// LED timestep function, which should be called approx. 100x per second.
void led_timestep() {
    if (!led_anim_type) {
        // LED_ANIM_TYPE_NONE
        return;
    }

    led_ring_anim_step++;
    if (led_ring_anim_step >= led_ring_anim_curr->speed) {
        // fade is complete. Time for the destination.
        led_ring_anim_step = 0;

        led_ring_anim_index++;

        // Go ahead and set our current color to the desired destination.
        //  This makes sure that we reach the _exact_ destination color every
        //  time, rather than opening ourselves up to propagation error.
        // TODO: index it based on animation type
        memcpy(&led_ring_curr, &led_ring_dest, sizeof(led_ring_curr));

        led_display_colors();

        // We conclude our current animation if, EITHER:
        //  a) We're looping with more loops left to go,
        //     and we've reached the end of the animation proper, OR
        //  b) We're not looping (or are, but have no loops left),
        //     and we've reached the end of the end padding.

        // We conclude the current cycle of the animation if the next shift
        //  will cause us to overflow off the end of the pad.
        if (led_ring_anim_index == led_ring_anim_curr->len + led_ring_anim_pad_len) {
            // animation is over.
            if (led_ring_anim_loops) {
                if (led_ring_anim_loops != 0xFF) {
                    led_ring_anim_loops--;
                }
                // We're not going to do either the start or end padding,
                //  since we're in an intermediate loop.
                // TODO:
//                led_ring_anim_index = led_ring_anim_pad_len;
                led_ring_anim_index = 0;
            } else {
                led_anim_type = LED_ANIM_TYPE_NONE;
                s_led_anim_done = 1;
                // No need to load any new colors, since the pad has taken care
                //  of turning all the lights off for us.
                return;
            }
        }

        // Stage the next color sets
        //  i.e., set dest and steps
        led_load_colors();

    } else {
        if (led_anim_type == LED_ANIM_TYPE_SAME) {
            led_ring_curr[0].r+= led_ring_step[0].r;
            led_ring_curr[0].g+= led_ring_step[0].g;
            led_ring_curr[0].b+= led_ring_step[0].b;
        } else { // if (led_anim_type == LED_ANIM_TYPE_SPIN) { // TODO
            for (uint8_t i=0; i<18; i++) {
                led_ring_curr[i].r+= led_ring_step[i].r;
                led_ring_curr[i].g+= led_ring_step[i].g;
                led_ring_curr[i].b+= led_ring_step[i].b;
            }
        }
        led_display_colors();
    }


}
