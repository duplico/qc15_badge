/*
 * leds.c
 *
 *  Created on: Jul 9, 2018
 *      Author: george
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "qc15.h"
#include "leds.h"

uint8_t s_led_anim_done = 0;

uint8_t led_anim_type = 0;
uint16_t led_ring_anim_step = 0;
uint8_t led_ring_anim_index = 0;
uint8_t led_ring_anim_loops = 0;
uint8_t led_ring_anim_num_colors = 0;
uint8_t led_ring_anim_len_padded = 0;
uint8_t led_ring_anim_pad_loops = 0;
led_ring_animation_t *led_ring_anim_curr;
led_ring_animation_t *led_ring_anim_bg = 0;
uint8_t led_ring_anim_pad_loops_bg = 0;
uint8_t led_anim_type_bg = 0;
rgbcolor16_t led_ring_curr[18];
rgbcolor16_t led_ring_dest[18];
rgbdelta_t led_ring_step[18];

uint8_t led_line_state = 0;
rgbcolor16_t led_line_curr[6] = {0,};
rgbcolor16_t led_line_dest[6];
rgbdelta_t led_line_step[6];

rgbcolor16_t led_line_centers[6] = {
    {255<<7, 0, 0},  // Red
    {255<<7, 20<<7, 0}, // Orange
    {255<<7, 60<<7, 0}, // Yellow
    {0, 64<<7, 0},   // Green
    {0, 0, 144<<7},  // Blue
    {128<<7, 0, 96<<7}, // Purple
};

void led_init() {
    memset(led_ring_curr, 0, sizeof(led_ring_curr));
    memset(led_ring_dest, 0, sizeof(led_ring_dest));
    memset(led_ring_step, 0, sizeof(led_ring_step));
}

const rgbcolor16_t color_off = {0, 0, 0};

/// Compute the index of the next frame in the current animation.
uint8_t next_anim_index(uint8_t index) {
    // If we're not looping, or if we're in the START pad,
    //  or if we're in the ANIMATION itself, then we just need to
    //  increment the index
    // (remember that the logic for reseting our index WON'T let us
    //  overflow here - see led_timestep())
    if (!led_ring_anim_loops || led_ring_anim_pad_loops ||
            (index+1 < led_ring_anim_curr->len + led_ring_anim_num_colors)) {
        return index+1;
    }

    // If we're down here, then the following things are true:
    //  1. We're in the END PAD.
    //  2. The animation is a loop.

    // This means that we need to decide whether to substitute in some
    //  of the animation (in the event that there are still remaining loops
    //  left to start), OR whether we just give it the end pad instead.

    // We're allowed to show, at most, this many extra animation frames:
    uint8_t extra_anim_frames = led_ring_anim_loops * led_ring_anim_curr->len;

    // We're this many frames into the END PAD:
    uint8_t index_into_pad = (index + 1) - (led_ring_anim_curr->len + led_ring_anim_num_colors);

    if (index_into_pad < extra_anim_frames) {
        // This one is OK to loop.
        return led_ring_anim_num_colors + ((index + 1) % led_ring_anim_curr->len);
    } else {
        // This one is just a pad.
        return index + 1;
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

    if (anim_index < led_ring_anim_num_colors ||
            anim_index >= led_ring_anim_curr->len + led_ring_anim_num_colors) {
        // If the current index is in the pad, i.e. before or after the anim,
        //  then turn the LED off until it's out of the pad.
        memcpy(dest_color_frame, &color_off, sizeof(rgbcolor16_t));
    } else {
        // Current index is in the animation, not the pad:
        unpadded_index = anim_index-led_ring_anim_num_colors;
        dest_color_frame->r = led_ring_anim_curr->colors[unpadded_index].r << 7;
        dest_color_frame->g = led_ring_anim_curr->colors[unpadded_index].g << 7;
        dest_color_frame->b = led_ring_anim_curr->colors[unpadded_index].b << 7;
    }
}

#pragma CODE_SECTION(led_load_colors,".run_from_ram")

/// Set up the current frame's color sets (i.e. dest and step).
/**
 ** This function is located in RAM because it has lots of low-performance
 **  and power-hungry division operations.
 */
void led_load_colors() {
    for (uint8_t i=0; i<led_ring_anim_num_colors; i++) {
        led_stage_color(&led_ring_dest[i],
                        next_anim_index(led_ring_anim_index+i));

        led_ring_step[i].r = ((int_fast16_t) led_ring_dest[i].r - (int_fast16_t)led_ring_curr[i].r) / led_ring_anim_curr->speed;
        led_ring_step[i].g = ((int_fast16_t) led_ring_dest[i].g - (int_fast16_t)led_ring_curr[i].g) / led_ring_anim_curr->speed;
        led_ring_step[i].b = ((int_fast16_t) led_ring_dest[i].b - (int_fast16_t)led_ring_curr[i].b) / led_ring_anim_curr->speed;
    }
}

/// Send the colors to the underlying controller.
void led_display_colors() {
    switch(led_anim_type) {
    case LED_ANIM_TYPE_SAME:
        ht16d_all_one_color_ring_only(led_ring_curr[0].r >> 7,
                                    led_ring_curr[0].g >> 7,
                                    led_ring_curr[0].b >> 7);
        break;
    case LED_ANIM_TYPE_FALL:
        for (uint8_t i=0; i<9; i++) {
            memcpy(&led_ring_curr[17-i], &led_ring_curr[i], sizeof(rgbcolor16_t));
        }
        // fall through...
    case LED_ANIM_TYPE_SPIN:
        ht16d_set_colors(0, 18, led_ring_curr);
        break;
    }
}

/// Set the current LED ring animation.
void led_set_anim(led_ring_animation_t *anim, uint8_t anim_type, uint8_t loops, uint8_t use_pad_in_loops) {
    if (led_ring_anim_loops == 0xFF && loops != 0xFF) {
        // If the current animation is loop-forever (background), and this
        //  one is not, then we should store it.
        led_ring_anim_bg = led_ring_anim_curr;
        led_ring_anim_pad_loops_bg = led_ring_anim_pad_loops;
    }
    led_ring_anim_curr = anim;
    led_anim_type = anim_type;
    led_ring_anim_step = 0;
    led_ring_anim_index = 0;
    led_ring_anim_loops = loops;

    if (anim_type == LED_ANIM_TYPE_SAME) {
        led_ring_anim_num_colors = 1;
    } else if (anim_type == LED_ANIM_TYPE_SPIN) {
        led_ring_anim_num_colors = 18;
    } else if (anim_type == LED_ANIM_TYPE_FALL) {
        led_ring_anim_num_colors = 9;
    }

    if (use_pad_in_loops > led_ring_anim_num_colors)
        led_ring_anim_pad_loops = led_ring_anim_num_colors;
    else
        led_ring_anim_pad_loops = use_pad_in_loops;

    // We need the full-sized pad on either side of the animation.
    //  In the event of looping animations, we will not use the pad
    //  between loops.
    led_ring_anim_len_padded = anim->len + 2*led_ring_anim_num_colors;

    // Set the current colors to their initial values. This is always
    //  going to be in the START pad (so they'll be blank)
    for (uint8_t i=0; i<led_ring_anim_num_colors; i++) {
        led_stage_color(&led_ring_curr[i], i);
    }

    // Set the destination and steps for each of the LEDs in play:
    led_load_colors();

    // Write the initial colors to the LED controller:
    led_display_colors();
}

uint8_t sleep_anim_type;

void led_off() {
    // TODO: sleep mode, if it exists
    ht16d_all_one_color(0,0,0); // Turn all LEDs off.
    sleep_anim_type = led_anim_type;
    led_anim_type = LED_ANIM_TYPE_NONE;
}

void led_on() {
    // TODO: Leave sleep mode, if it exists
    led_anim_type = sleep_anim_type;
}

/// LED timestep function, which should be called approx. 30x per second.
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
        memcpy(&led_ring_curr, &led_ring_dest, sizeof(rgbcolor16_t) * led_ring_anim_num_colors);

        led_display_colors();

        // We conclude our current animation if, EITHER:
        //  a) We're looping with more loops left to go,
        //     and we've reached the end of the animation proper, OR
        //  b) We're not looping (or are, but have no loops left),
        //     and we've reached the end of the end padding.

        // We conclude the current cycle of the animation if the next shift
        //  will cause us to overflow off the end of the pad.
        if (led_ring_anim_index == led_ring_anim_curr->len + led_ring_anim_num_colors) {
            // animation is over.
            if (led_ring_anim_loops) {
                if (led_ring_anim_loops != 0xFF) {
                    led_ring_anim_loops--;
                }
                // Generally speaking, when we loop we'll skip the START pad
                //  in between loop iterations. However, the variable
                //  led_ring_anim_pad_loops forces us to include a pad
                //  between loops, which will use the START pad.
                if (led_ring_anim_pad_loops) {
                    led_ring_anim_index = 0;
                } else {
                    led_ring_anim_index = led_ring_anim_num_colors;
                }
            } else {
                if (led_ring_anim_bg) {
                    // If we have a background animation stored, that this one
                    //  was briefly superseding, then go ahead and start it
                    //  up again.
                    led_set_anim(led_ring_anim_bg, led_anim_type_bg,
                                 0xff, led_ring_anim_pad_loops_bg);
                } else {
                    led_anim_type = LED_ANIM_TYPE_NONE;
                    s_led_anim_done = 1;
                    // No need to load any new colors, since the pad has taken care
                    //  of turning all the lights off for us.
                }
                return;
            }
        }

        // Stage the next color sets
        //  i.e., set dest and steps
        led_load_colors();

    } else {
        for (uint8_t i=0; i<led_ring_anim_num_colors; i++) {
            led_ring_curr[i].r+= led_ring_step[i].r;
            led_ring_curr[i].g+= led_ring_step[i].g;
            led_ring_curr[i].b+= led_ring_step[i].b;
        }
        led_display_colors();
    }


}
