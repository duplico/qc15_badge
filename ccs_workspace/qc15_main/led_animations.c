/*
 * led_animations.c
 *
 *  Created on: Jul 9, 2018
 *      Author: george
 */

// Flags:
#include "leds.h"
#include "state_definitions.h"
#include "led_animations.h"

#define SPIN_SPEED 2
#define SAME_SPEED 32

const rgbcolor_t flag_rainbow_colors[] = {
      {255, 0, 0}, // Red
      {255, 24, 0x00}, // Orange
      {128, 40, 0x00}, // Yellow
      {0, 64, 0}, // Green
      {0, 0, 196}, // Blue
      {128, 0, 128}, // Purple
};

const rgbcolor_t flag_bi_colors[] = {
        {0xff, 0x00, 0xb0},
        {0,0,0},
        {0,0,0},
        {0x50, 0x00, 0xff},
};
const rgbcolor_t flag_pan_colors[] = {
        {0xff, 0x21, 0x8c},
        {0xff, 0xd8, 0x00},
        {0xff, 0xd8, 0x00},
        {0x21, 0xb1, 0xff},
};

const rgbcolor_t flag_trans_colors[] = {
        {0x1B, 0xCE, 0xFA},
        {0x1B, 0xCE, 0xFA},
        {0xff, 0x00, 0xb0},
        {0xFF, 0xFF, 0xFF},
        {0xFF, 0xFF, 0xFF},
        {0xff, 0x00, 0xb0},
        {0x1B, 0xCE, 0xFA},
        {0x1B, 0xCE, 0xFA},
};
const rgbcolor_t flag_ace_colors[] = {
        {0x70, 0x00, 0xFF},
        {0x70, 0x00, 0xFF},
        {0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00},
        {0xFF, 0xFF, 0xFF},
        {0x03, 0x03, 0x03},
        {0x03, 0x03, 0x03},
        {0x00, 0x00, 0x00},
        {0x03, 0x03, 0x03},
};
const rgbcolor_t flag_ally_colors[] = {
        {0x00, 0x00, 0x00},
        {0x5F, 0x5F, 0x5F},
        {0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00},
        {0x5F, 0x5F, 0x5F},
        {0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00},
        {0x5F, 0x5F, 0x5F},
        {0x00, 0x00, 0x00},
};

const rgbcolor_t flag_leather_colors[] = {
        {0x08, 0x08, 0x6B},
        {0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00},
        {0x08, 0x08, 0x6B},
        {0x08, 0x08, 0x6B},
        {0xFF, 0xFF, 0xFF},
        {0x08, 0x08, 0x6B},
        {0x08, 0x08, 0x6B},
        {0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00},
        {0xFF, 0x00, 0x00},
        {0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00},
};
const rgbcolor_t flag_bear_colors[] = {
        {0xA5, 0x38, 0x04},
        {0xD5, 0x63, 0x00},
        {0xD5, 0x63, 0x00},
        {0xD5, 0x63, 0x00},
        {0xFE, 0xDD, 0x63},
        {0xFE, 0xDD, 0x63},
        {0xbE, 0xa6, 0x38},
        {0x00, 0x00, 0x00},
};

const rgbcolor_t flag_blue_colors[] = {
        {0x00, 0x00, 0xFF},
};
const rgbcolor_t flag_lblue_colors[] = {
        {0x00, 0xC0, 0xFF},
};
const rgbcolor_t flag_green_colors[] = {
        {0x0F, 0xFF, 0x07},
};

const rgbcolor_t flag_red_colors[] = {
        {0xFF, 0x00, 0x00},
};
const rgbcolor_t flag_yellow_colors[] = {
        {0xE0, 0xA0, 0x00},
};

const rgbcolor_t flag_pink_colors[] = {
        {0xd5, 0x00, 0x69},
};

// End of flag colors

const rgbcolor_t flag_white_colors[] = {
        {255, 255, 255},
        {255, 255, 255},
        {255, 255, 255},
        {0, 0, 0},
        {255, 255, 255},
        {255, 255, 255},
        {0, 0, 0},
        {255, 255, 255},
        {255, 255, 255},
        {255, 255, 255},
        {0, 0, 0},
        {255, 255, 255},
};

const rgbcolor_t lwf_colors[] = {
        {32, 32, 32},
        {96, 96, 96},
        {128, 128, 128},
        {255, 255, 255},
        {128, 128, 128},
        {96, 96, 96},
        {32, 32, 32},
        {0, 0, 0},
};

const rgbcolor_t blue_colors[] = {
        {0, 0, 255},
        {0, 0, 255},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 255},
        {0, 0, 255},
};

const rgbcolor_t whitediscovery_colors[] = {
        {255, 255, 255},
        {0, 0, 0},
        {64, 64, 64},
};

const rgbcolor_t orange_colors[] = {
        {255, 24, 0},
        {255, 24, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {255, 24, 0},
        {255, 24, 0},
};

const rgbcolor_t green_colors[] = {
        {0, 64, 0},
        {0, 64, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 64, 0},
        {0, 64, 0},
};

const rgbcolor_t yellow_colors[] = {
        {128, 40, 0},
        {128, 40, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {128, 40, 0},
        {128, 40, 0},
};

const rgbcolor_t red_colors[] = { // "light white fade" (normal)
        {255, 0, 0},
        {255, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {255, 0, 0},
        {255, 0, 0},
};

const rgbcolor_t white_colors[] = { // "light white fade" (normal)
        {255, 255, 255},
        {255, 255, 255},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {255, 255, 255},
        {255, 255, 255},
};

const rgbcolor_t pink_colors[] = { // "light white fade" (normal)
        {128, 0, 128},
        {128, 0, 128},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {128, 0, 128},
        {128, 0, 128},
};

/// Special case fall-back animation.
const led_ring_animation_t anim_rainbow_spin = {
        &flag_rainbow_colors[0],
        6,
        5,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SPIN,
};

// The first FLAG_COUNT of these are the flag ones.
const led_ring_animation_t all_animations[GAME_ANIMS_LEN] = {
    {
     &flag_rainbow_colors[0],
     6,
     DEFAULT_FLAG_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Rainbow"
    },
    {
     &flag_bi_colors[0],
     4,
     DEFAULT_FLAG_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Bisexual"
    },
    {
     &flag_pan_colors[0],
     4,
     DEFAULT_FLAG_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Pansexual"
    },
    {
     &flag_trans_colors[0],
     8,
     DEFAULT_FLAG_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Trans",
    },
    {
     &flag_ace_colors[0],
     9,
     DEFAULT_FLAG_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Asexual",
    },
    {
     &flag_ally_colors[0],
     9,
     DEFAULT_FLAG_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Ally",
    },
     {
      &flag_leather_colors[0],
      14,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Leather"
     },
     {
      &flag_bear_colors[0],
      8,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Bear"
     },
     {
      &flag_blue_colors[0],
      1,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Blue"
     },
     {
      &flag_lblue_colors[0],
      1,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Light blue"
     },
     {
      &flag_green_colors[0],
      1,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Green"
     },
     {
      &flag_red_colors[0],
      1,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Red"
     },
     {
      &flag_yellow_colors[0],
      1,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Yellow"
     },
     {
      &flag_pink_colors[0],
      1,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Pink"
     },
    { // "light solid white" (bright) // TODO: CHANGE
      &flag_white_colors[0],
      1,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "White"
    },
    { // Newbie flag TODO
      &flag_white_colors[0],
      12,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "QC Newbie"
    },
    { // Original flag TODO
      &flag_white_colors[0],
      12,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "QC Original"
    },
    { // Regular flag TODO
      &flag_white_colors[0],
      12,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "QC Regular"
    },
    { // Freezer flag TODO
      &flag_white_colors[0],
      12,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Frozen!"
    },
    { // Tech support flag TODO
      &flag_white_colors[0],
      12,
      DEFAULT_FLAG_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Tech support"
    },

    // End of flags, start of others:

    { // The first light animation
      &flag_white_colors[0],
      12,
      4,
      HT16D_BRIGHTNESS_MAX,
      LED_ANIM_TYPE_SPIN,
      "firstLights"
    },
//    { // The calmer version of the first lights.
//     &lwf_colors[0], // TODO TODO TODO TODO
//     8,
//     16,
//     HT16D_BRIGHTNESS_DEFAULT,
//     LED_ANIM_TYPE_FALL,
//     "whitefade"
//    },
    { // ATTN ATTN ATTN!!!
     &whitediscovery_colors[0],
     3,
     6,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_SAME,
     "whitedisc"
    },
    {
     &blue_colors[0],
     11,
     SPIN_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_SPIN,
     "spinBlue"
    },(led_ring_animation_t) {
         .colors = &orange_colors[0],
         .len = 11,
         .speed = SPIN_SPEED,
         .brightness = HT16D_BRIGHTNESS_DEFAULT,
         .type = LED_ANIM_TYPE_SPIN,
         .name = "spinOrange"
     },
     (led_ring_animation_t) {
         .colors = &yellow_colors[0],
         .len = 11,
         .speed = SPIN_SPEED,
         .brightness = HT16D_BRIGHTNESS_DEFAULT,
         .type = LED_ANIM_TYPE_SPIN,
         .name = "spinYellow"
     },
     (led_ring_animation_t) {
         .colors = &green_colors[0],
         .len = 11,
         .speed = SPIN_SPEED,
         .brightness = HT16D_BRIGHTNESS_DEFAULT,
         .type = LED_ANIM_TYPE_SPIN,
         .name = "spinGreen"
     },
     (led_ring_animation_t) {
         .colors = &red_colors[0],
         .len = 11,
         .speed = SPIN_SPEED,
         .brightness = HT16D_BRIGHTNESS_DEFAULT,
         .type = LED_ANIM_TYPE_SPIN,
         .name = "spinRed"
     },
     (led_ring_animation_t) {
         .colors = &white_colors[0],
         .len = 11,
         .speed = SPIN_SPEED,
         .brightness = HT16D_BRIGHTNESS_DEFAULT,
         .type = LED_ANIM_TYPE_SPIN,
         .name="spinWhite"
     },
     (led_ring_animation_t) {
         .colors = &pink_colors[0],
         .len = 11,
         .speed = SPIN_SPEED,
         .brightness = HT16D_BRIGHTNESS_DEFAULT,
         .type = LED_ANIM_TYPE_SPIN,
         .name="spinPink"
     },
    (led_ring_animation_t) {
        .colors = &blue_colors[0],
        .len = 2,
        .speed = SAME_SPEED,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
        .name = "solidBlue"
    },
    (led_ring_animation_t) {
        .colors = &green_colors[0],
        .len = 2,
        .speed = SAME_SPEED,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
        .name = "solidGreen"
    },
    (led_ring_animation_t) {
        .colors = &yellow_colors[0],
        .len = 2,
        .speed = SAME_SPEED,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
        .name = "solidYellow"
    },
    (led_ring_animation_t) {
        .colors = &orange_colors[0],
        .len = 2,
        .speed = SAME_SPEED,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
        .name = "solidOrange"
    },
    (led_ring_animation_t) {
        .colors = &red_colors[0],
        .len = 2,
        .speed = SAME_SPEED,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
        .name = "solidRed"
    },
    (led_ring_animation_t) {
        .colors = &white_colors[0],
        .len = 2,
        .speed = SAME_SPEED,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
        .name = "solidWhite"
    },
};

/////////////////////////////////////////////////////////////
