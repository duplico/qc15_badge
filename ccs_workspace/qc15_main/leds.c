/// High-level module to handle LED animations.
/**
 ** \file leds.c
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "qc15.h"
#include "leds.h"

/// Signal to the main loop that a temporary animation has finished.
uint8_t s_led_anim_done = 0;

/// The type of LED ring animation: FALL, RING, SAME, or NONE.
uint8_t led_anim_type = 0;
/// Our position within the current frame transition (fade).
uint16_t led_ring_anim_step = 0;
/// Our frame in the current animation (which may include 0-padding)
uint8_t led_ring_anim_index = 0;
/// The number of remaining loops in the animation, or 0xFF for background.
uint8_t led_ring_anim_loops = 0;
/// The number of unique colors we are set up to do in our animation type.
uint8_t led_ring_anim_num_leds = 0;
/// The number of total frames in the animation, plus some 0-padding.
uint8_t led_ring_anim_len_padded = 0;
///
uint8_t led_ring_anim_pad_loops = 0;
/// Pointer to the current animation.
const led_ring_animation_t *led_ring_anim_curr;
/// Pointer to saved animation (if we're doing a temporary one).
const led_ring_animation_t *led_ring_anim_bg = 0;
/// Saved value of `led_ring_anim_pad_loops` for backgrounds.
uint8_t led_ring_anim_pad_loops_bg = 0;
/// Saved value of `led_anim_type` for backgrounds.
uint8_t led_anim_type_bg = 0;

/// The current colors of the LED ring.
rgbcolor16_t led_ring_curr[18];
/// The next frame colors (destination colors) of the LED ring.
rgbcolor16_t led_ring_dest[18];
/// The amount to change the ring's value every step.
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

const rgbcolor16_t color_off = {0, 0, 0};

void led_init() {
    memset(led_ring_curr, 0, sizeof(led_ring_curr));
    memset(led_ring_dest, 0, sizeof(led_ring_dest));
    memset(led_ring_step, 0, sizeof(led_ring_step));
}

/// Compute the index of the next frame in the current animation.
uint8_t next_anim_index(uint8_t index) {
    // If we're not looping, or if we're in the START pad,
    //  or if we're in the ANIMATION itself, then we just need to
    //  increment the index
    return (index + 1) % led_ring_anim_len_padded;
            // TODO:

    // (remember that the logic for reseting our index WON'T let us
    //  overflow here - see led_timestep())
    if (!led_ring_anim_loops || led_ring_anim_pad_loops ||
            (index+1 < led_ring_anim_curr->len + led_ring_anim_num_leds)) {
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
    uint8_t index_into_pad = (index + 1) - (led_ring_anim_curr->len + led_ring_anim_num_leds);

    if (index_into_pad < extra_anim_frames) {
        // This one is OK to loop.
        return led_ring_anim_num_leds + ((index + 1) % led_ring_anim_curr->len);
    } else {
        // This one is just a pad.
        return index + 1;
    }
}

/// Place colors into a color frame element, accounting for padding.
/**
 ** Taking in both the frame index, and the led number, determine what color
 ** that LED should be at that frame, and place that color into the color
 ** frame pointed to by the first parameter.
 **
 ** We do this by basically interpreting the LED index as a "look back"
 ** distance from a frame index, to get a color index. So, modulo the
 ** padded length of our animation, `color_index = frame_index - led_index`.
 **
 ** An illustrative example:
 **
 ** * On frame[0], color[0] goes on led[0] and led[1] gets color[-1].
 **
 ** * But on frame[1], color[0] goes on led[1] and led[0] gets color[1].
 **
 ** So the math works!
 **
 **/
void led_stage_color(rgbcolor16_t *dest_color_frame, uint8_t frame_index,
                     uint8_t led_index) {
    uint16_t color_index = 0;

    if (frame_index >= led_index) {
        color_index = frame_index - led_index;
    } else {
        // frame_index - led_number < 0, so:
        color_index = led_ring_anim_len_padded + frame_index - led_index;
    }

    if (color_index >= led_ring_anim_curr->len) {
        // It's off in the pad.
        memcpy(dest_color_frame, &color_off, sizeof(rgbcolor16_t));
    } else {
        // It's a color!
        dest_color_frame->r = led_ring_anim_curr->colors[color_index].r << 7;
        dest_color_frame->g = led_ring_anim_curr->colors[color_index].g << 7;
        dest_color_frame->b = led_ring_anim_curr->colors[color_index].b << 7;
    }
}

#pragma CODE_SECTION(led_load_colors,".run_from_ram")

/// Set up the current frame's color sets (i.e. dest and step).
/**
 ** This function is located in RAM because it has lots of low-performance
 **  and power-hungry division operations.
 */
void led_load_colors() {
    for (uint8_t i=0; i<led_ring_anim_num_leds; i++) {
        led_stage_color(&led_ring_dest[i],
                        next_anim_index(led_ring_anim_index),
                        i);

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

void led_set_anim_none() {
    led_anim_type = LED_ANIM_TYPE_NONE;
    led_ring_anim_loops = 0;
    ht16d_all_one_color_ring_only(0, 0, 0);
    // TODO: Does this need to signal? Probably no...
}

/// Set the current LED ring animation.
/**
 ** \param anim Pointer to the animation to load.
 ** \param anim_type The type of the animation, which may be
 **                  LED_ANIM_TYPE_SPIN, LED_ANIM_TYPE_SAME, or
 **                  LED_ANIM_TYPE_FALL to override the default animation
 **                  type of the animation. It may also be LED_ANIM_TYPE_NONE,
 **                  or any other False-evaluating value to avoid overriding
 **                  the animation's default type.
 ** \param loops The number of times to play the animation. As a special case,
 **              setting loops to 0xFF will cause the animation to become the
 **              new background animation, looping indefinitely. Values of
 **              0 and 1 for `loops` have the same effect.
 ** \param use_pad_in_loops
 */
void led_set_anim(const led_ring_animation_t *anim, uint8_t anim_type,
                  uint8_t loops, uint8_t use_pad_in_loops) {
    if (led_ring_anim_loops == 0xFF && loops != 0xFF) {
        // If the current animation is loop-forever (background), and this
        //  one is not, then we should store it.
        led_ring_anim_bg = led_ring_anim_curr;
        led_ring_anim_pad_loops_bg = led_ring_anim_pad_loops;
        led_anim_type_bg = led_anim_type;
    }
    led_ring_anim_curr = anim;
    led_anim_type = anim_type? anim_type : led_ring_anim_curr->type;
    led_ring_anim_step = 0;
    led_ring_anim_loops = loops? loops : 1; // No 0 loops allowed. See below.
    // We don't allow "0" loops for two reasons:
    //  1. In order to have a blank-padded entry to the animation, we actually
    //     start the animation at the final frame index. So if we loop 0 times,
    //     the animation won't do anything.
    //  2. It actually turns out to be super confusing that, in order to get
    //     an animation to play once, you have to pass an argument of 0. So
    //     this way you get it with a 1.

    if (led_anim_type == LED_ANIM_TYPE_SAME) {
        led_ring_anim_num_leds = 1;
    } else if (led_anim_type == LED_ANIM_TYPE_SPIN) {
        led_ring_anim_num_leds = 18;
    } else if (led_anim_type == LED_ANIM_TYPE_FALL) {
        led_ring_anim_num_leds = 9;
    }

    // This allows a custom padding size:
    led_ring_anim_pad_loops = use_pad_in_loops? use_pad_in_loops :
                                                led_ring_anim_num_leds;

    // Here, we apply a pad to the animation. This is a number of dummy
    //  colors at the end of the list of colors, which the function
    //  `led_stage_color()` interprets as OFF.
    // Sometimes, our looping logic may allow us to skip some or all of the
    //  padding colors.
    led_ring_anim_len_padded = led_ring_anim_curr->len + led_ring_anim_num_leds;

    // We don't allow animations to interleave with each other, so our first
    //  step is always going to be to set the current colors to OFF.
    // We do this by selecting a fake frame index below so that it's
    //  the first dummy color in the pad at the end of the color array:
    led_ring_anim_index = led_ring_anim_len_padded-1;
    for (uint8_t led=0; led<led_ring_anim_num_leds; led++) {
        led_stage_color(&led_ring_curr[led], led_ring_anim_index, led);
    }

    // Set the destination and steps for each of the LEDs in play:
    led_load_colors();

    // Write the initial colors to the LED controller:
    led_display_colors();
    // Now, the LEDs are blank. Make our brightness change.
    ht16d_set_global_brightness(led_ring_anim_curr->brightness);
}

uint8_t sleep_anim_type;

void led_off() {
    // TODO: sleep mode, if it exists
    ht16d_all_one_color(0,0,0); // Turn all LEDs off.
    sleep_anim_type = led_anim_type;
    led_anim_type = LED_ANIM_TYPE_NONE;
}

void led_on() {
    // TODO: We need a way to tell whether we're currently sleeping.
    if (sleep_anim_type)
        led_anim_type = sleep_anim_type;
}

/// LED timestep function, which should be called 32x per second.
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
        memcpy(&led_ring_curr, &led_ring_dest,
               sizeof(rgbcolor16_t) * led_ring_anim_num_leds);

        led_display_colors();

        // We conclude our current animation if, EITHER:
        //  a) We're looping with more loops left to go,
        //     and we've reached the end of the animation proper, OR
        //  b) We're not looping (or are, but have no loops left),
        //     and we've reached the end of the end padding.

        // We conclude the current cycle of the animation if the next shift
        //  will cause us to overflow off the end of the pad.
        if (led_ring_anim_index == led_ring_anim_len_padded) {
            // animation is over.
            if (led_ring_anim_loops) {
                if (led_ring_anim_loops != 0xFF) {
                    led_ring_anim_loops--;
                }
                // Loop!
                led_ring_anim_index = 0;
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
        // Still fading.

        for (uint8_t i=0; i<led_ring_anim_num_leds; i++) {
            led_ring_curr[i].r+= led_ring_step[i].r;
            led_ring_curr[i].g+= led_ring_step[i].g;
            led_ring_curr[i].b+= led_ring_step[i].b;
        }
        led_display_colors();
    }


}
