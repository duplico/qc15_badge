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
#include "codes.h"
#include "leds.h"
#include "badge.h"
#include "util.h"

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
/// Custom pad length.
uint8_t led_ring_anim_pad_loops = 0;
///
uint8_t led_ring_anim_elapsed = 0;

/// Pointer to the current animation.
const led_ring_animation_t *led_ring_anim_curr;

/// The current colors of the LED ring.
rgbcolor16_t led_ring_curr[18];
/// The next frame colors (destination colors) of the LED ring.
rgbcolor16_t led_ring_dest[18];
/// The amount to change the ring's value every step.
rgbdelta_t led_ring_step[18];

#define LED_LINE_STEPS_PER_FRAME 32

uint8_t led_line_frame = 0;
uint8_t led_line_frame_step = 0;
uint8_t led_line_offset[6] = {0,};
int8_t led_line_direction[6] = {1,1,1,1,1,1};
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

const rgbcolor16_t colors_dim[6] = {
    {255<<4, 0, 0},  // Red
    {255<<4, 20<<5, 0}, // Orange
    {255<<4, 60<<5, 0}, // Yellow
    {0, 64<<4, 0},   // Green
    {0, 0, 144<<4},  // Blue
    {128<<4, 0, 96<<4}, // Purple
};

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

    // Here, we do a little bit of sneakiness. We want to shift the pad around,
    //  just a bit, so that at index 0 everything is dark.
    // So if we're passed frame_index 0, we convert it to frame_index
    //  led_ring_anim_len_padded-1. (basically, we subtract 1 modulo
    //  led_ring_anim_len_padded).

    frame_index = (frame_index + led_ring_anim_len_padded - 1)
                                            % led_ring_anim_len_padded;

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

/// Set up the current frame's color sets (i.e. dest and step).
/**
 ** This function is located in RAM because it has lots of low-performance
 **  and power-hungry division operations.
 */
__attribute__((ramfunc))
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
        ht16d_all_one_color_ring_only(led_ring_curr[0].r >> 8,
                                    led_ring_curr[0].g >> 8,
                                    led_ring_curr[0].b >> 8);
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

    // Clear the background animation, too.
    led_ring_anim_bg = 0;
}

/// Set the current LED ring animation.
/**
 ** By default, this will attempt to insert a sane length of 0-padding between
 ** loops of the animation, so that there will be an optimal number of OFF
 ** LEDs between instances of the animation. The behavior is as follows:
 **
 ** * LED_ANIM_TYPE_SAME: Padding of 1 OFF frame between animations
 **
 ** * LED_ANIM_TYPE_SPIN: Padding set such that the chase will appear
 **                       continuous
 **
 ** * LED_ANIM_TYPE_FALL: Padding length of 5 (the number of LEDs down the
 **                       side).
 **
 ** This behavior can be overridden with the `extra_padding` parameter, but
 ** overriding it may cause undesired behavior. See the description of the
 ** parameter for more details on the constraints.
 **
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
 ** \param extra_padding If nonzero, override the default auto-padding feature
 **              and use the value of extra_padding as the pad. Note that,
 **              unless animation length + pad length >= number of LEDs active
 **              in the animation, the appearance of the first loop and last
 **              loop (if applicable) may not be satisfactory. Also, only up to
 **              two copies of the animation are drawn on the LEDs at once,
 **              so if ``(2*len)+pad < num_leds`` where ``num_leds`` is 9
 **              for ``FALL`` moed and 18 for ``SPIN`` mode, the appearance
 **              will not look correct at all.
 **
 */
void led_set_anim(const led_ring_animation_t *anim, uint8_t anim_type,
                  uint8_t loops, uint8_t extra_padding) {
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
    led_ring_anim_index = 0;

    if (loops && loops != 0xFF) {
        loops--; // It's confusing for a param 1 to play the animation twice.
    }
    led_ring_anim_loops = loops;

    if (led_anim_type == LED_ANIM_TYPE_SAME) {
        led_ring_anim_num_leds = 1;
    } else if (led_anim_type == LED_ANIM_TYPE_SPIN) {
        led_ring_anim_num_leds = 18;
    } else if (led_anim_type == LED_ANIM_TYPE_FALL) {
        led_ring_anim_num_leds = 9;
    }

    // Padding automation and override:
    if (extra_padding) {
        led_ring_anim_pad_loops = extra_padding;
    } else if (led_anim_type == LED_ANIM_TYPE_SAME) {
        led_ring_anim_pad_loops = 1;
    } else if (led_anim_type == LED_ANIM_TYPE_SPIN){
        led_ring_anim_pad_loops = led_ring_anim_num_leds -
                (led_ring_anim_curr->len % led_ring_anim_num_leds);
    } else {
        // waterfall
        if (led_ring_anim_curr->len >=5)
            led_ring_anim_pad_loops = 4;
        else
            led_ring_anim_pad_loops = led_ring_anim_num_leds - led_ring_anim_curr->len;
    }

    // Here, we apply a pad to the animation. This is a number of dummy
    //  colors at the end of the list of colors, which the function
    //  `led_stage_color()` interprets as OFF.
    // Sometimes, our looping logic may allow us to skip some or all of the
    //  padding colors.
    led_ring_anim_len_padded = led_ring_anim_curr->len + led_ring_anim_num_leds;

    // We don't allow animations to interleave with each other, so our first
    //  step is always going to be to set the current colors to OFF.
    // This is taken care of by our padding, and some tricky logic inside of
    //  led_stage_color().
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
    ht16d_all_one_color(0,0,0); // Turn all LEDs off.
    sleep_anim_type = led_anim_type;
    led_anim_type = LED_ANIM_TYPE_NONE;
}

void led_on() {
    if (sleep_anim_type)
        led_anim_type = sleep_anim_type;
}

void led_ring_timestep() {
    if (!led_anim_type) {
        // LED_ANIM_TYPE_NONE
        return;
    }

    led_ring_anim_step++;
    if (led_ring_anim_step >= led_ring_anim_curr->speed) {
        // fade is complete. Time for the destination.
        led_ring_anim_step = 0;

        led_ring_anim_index++;

        if (led_ring_anim_pad_loops && led_ring_anim_loops &&
                led_ring_anim_index == led_ring_anim_curr->len) {
            led_ring_anim_len_padded = led_ring_anim_curr->len + led_ring_anim_pad_loops;
        } else if (led_ring_anim_pad_loops &&
                led_ring_anim_index == led_ring_anim_curr->len) {
            led_ring_anim_len_padded = led_ring_anim_curr->len + led_ring_anim_num_leds;
        }

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

uint8_t led_line_next_offset(uint8_t file_id) {
    uint8_t file_id_min;
    uint8_t file_id_max;
    uint8_t file_id_center = file_id + 6;
    uint8_t file_id_offset = led_line_offset[file_id];
    uint16_t distance = 0;
    if (is_solved(file_id)) {
        // DIFFERENT BEHAVIOR
        return file_id;
    }

    // Calculate how far away we are (distance /5 + 1)
    distance = (5 - 5 * buffer_rank(badge_conf.code_part_unlocks[file_id], CODE_SEGMENT_REP_LEN) / 80) + 1;
    file_id_min = file_id_center - distance;
    file_id_max = file_id_center + distance;

    if (file_id_offset > file_id_max)
        return file_id_max;
    else if (file_id_offset == file_id_max)
        return file_id_max-1;
    else if (file_id_offset < file_id_min)
        return file_id_min;
    else if (file_id_offset == file_id_min)
        return file_id_min+1;
    else
        return file_id_offset + led_line_direction[file_id];
}

void led_activate_file_lights() {
    led_line_frame_step = 0;
    led_line_frame = 0;
    for (uint8_t i=0; i<6; i++) {
        led_line_offset[i] = 6; // = 0 % 6, but 6 so we can go "negative"
    }

    for (uint8_t i=0; i<6; i++) {
        memcpy(&led_line_curr[i], &color_off, sizeof(rgbcolor16_t));
        memcpy(&led_line_dest[i], &led_line_centers[led_line_next_offset(i)%6], sizeof(rgbcolor16_t));

        led_line_step[i].r = ((int_fast16_t) led_line_dest[i].r - (int_fast16_t)led_line_curr[i].r) / LED_LINE_STEPS_PER_FRAME;
        led_line_step[i].g = ((int_fast16_t) led_line_dest[i].g - (int_fast16_t)led_line_curr[i].g) / LED_LINE_STEPS_PER_FRAME;
        led_line_step[i].b = ((int_fast16_t) led_line_dest[i].b - (int_fast16_t)led_line_curr[i].b) / LED_LINE_STEPS_PER_FRAME;
    }
}

void led_line_timestep() {
    led_line_frame_step++;
    if (led_line_frame_step == LED_LINE_STEPS_PER_FRAME) {
        for (uint8_t i=0; i<6; i++) {
            led_line_curr[i] = led_line_dest[i];
        }

        led_line_frame_step = 0;
        led_line_frame++;

        for (uint8_t i=0; i<6; i++) {
            if (is_solved(i)) {
                // Special case.
                memcpy(&led_line_dest[i], ((led_line_frame) & 0x01) ? &led_line_centers[i] : &colors_dim[i], sizeof(rgbcolor16_t));
            } else {

                uint8_t next_offset = led_line_next_offset(i);

                if (next_offset < led_line_offset[i])
                    led_line_direction[i] = -1;
                else
                    led_line_direction[i] = 1;

                led_line_offset[i] = next_offset;

                memcpy(&led_line_dest[i], &led_line_centers[led_line_offset[i]%6], sizeof(rgbcolor16_t));
            }

            led_line_step[i].r = ((int_fast16_t) led_line_dest[i].r - (int_fast16_t)led_line_curr[i].r) / LED_LINE_STEPS_PER_FRAME;
            led_line_step[i].g = ((int_fast16_t) led_line_dest[i].g - (int_fast16_t)led_line_curr[i].g) / LED_LINE_STEPS_PER_FRAME;
            led_line_step[i].b = ((int_fast16_t) led_line_dest[i].b - (int_fast16_t)led_line_curr[i].b) / LED_LINE_STEPS_PER_FRAME;
        }

    } else {
        for (uint8_t i=0; i<6; i++) {
            led_line_curr[i].r+= led_line_step[i].r;
            led_line_curr[i].g+= led_line_step[i].g;
            led_line_curr[i].b+= led_line_step[i].b;
        }
    }
    ht16d_set_colors(18, 6, led_line_curr);
}

/// LED timestep function, which should be called 32x per second.
void led_timestep() {
    led_ring_timestep();
    if (badge_conf.file_lights_on)
        led_line_timestep();
}
