/*
 * led_animations.c
 *
 *  Created on: Jul 9, 2018
 *      Author: george
 */

// Flags:
#include "leds.h"

#define FLAG_COUNT 14

const rgbcolor_t flag_rainbow_colors[] = {
        {0xe4, 0x03, 0x03}, // Red
        {0xff, 0x8c, 0x00}, // Orange
        {0xff, 0xed, 0x00}, // Yellow
        {0x00, 0x80, 0x26}, // Green
        {0x00, 0x4d, 0xff}, // Blue
        {0x75, 0x07, 0x87}, // Purple
};

const led_ring_animation_t flag_rainbow = {
        &flag_rainbow_colors[0],
        6,
        DEFAULT_ANIM_SPEED,
        "Rainbow"
};

const rgbcolor_t flag_bi_colors[] = {
        {0xff, 0x00, 0xb0},
        {0,0,0},
//        {0x5300, 0x1f00, 0x6600}, // 115, 79, 150
        {0,0,0},
        {0x50, 0x00, 0xff}, // 0, 56, 168
};

const led_ring_animation_t flag_bi = {
        &flag_bi_colors[0],
        4,
        DEFAULT_ANIM_SPEED,
        "Bisexual"
};

const rgbcolor_t flag_pan_colors[] = {
        {0xff, 0x21, 0x8c}, // 255,33,140
        {0xff, 0xd8, 0x00}, //255,216,0
        {0xff, 0xd8, 0x00}, //255,216,0
        {0x21, 0xb1, 0xff}, //33,177,255
};

const led_ring_animation_t flag_pan = {
        &flag_pan_colors[0],
        4,
        DEFAULT_ANIM_SPEED,
        "Pansexual"
};

const rgbcolor_t flag_trans_colors[] = {
        {0x1B, 0xCE, 0xFA}, //91,206,250
        {0x1B, 0xCE, 0xFA}, //91,206,250
//        {0xFF00, 0x8900, 0x9800}, //245,169,184
        {0xff, 0x00, 0xb0},
        {0xFF, 0xFF, 0xFF}, //255,255,255
        {0xFF, 0xFF, 0xFF}, //255,255,255
//        {0xFF00, 0xA900, 0xB800}, //245,169,184
        {0xff, 0x00, 0xb0},
        {0x1B, 0xCE, 0xFA}, //91,206,250
        {0x1B, 0xCE, 0xFA}, //91,206,250
};

const led_ring_animation_t flag_trans = {
        &flag_trans_colors[0],
        8,
        DEFAULT_ANIM_SPEED,
        "Trans"
};

const rgbcolor_t flag_ace_colors[] = {
        {0x70, 0x00, 0xFF}, //128,0,128
        {0x70, 0x00, 0xFF}, //128,0,128
        {0x00, 0x00, 0x00}, //128,0,128
        {0x00, 0x00, 0x00}, //128,0,128
        {0xFF, 0xFF, 0xFF}, //255,255,255
        {0x03, 0x03, 0x03}, //163,163,163,
        {0x03, 0x03, 0x03}, //163,163,163,
        {0x00, 0x00, 0x00}, //163,163,163,
        {0x03, 0x03, 0x03}, //163,163,163,
};

const led_ring_animation_t flag_ace = {
        &flag_ace_colors[0],
        9,
        DEFAULT_ANIM_SPEED,
        "Asexual"
};

const rgbcolor_t flag_ally_colors[] = {
        {0x00, 0x00, 0x00}, //0,0,0
        {0x5F, 0x5F, 0x5F}, //255,255,255
        {0x00, 0x00, 0x00}, //0,0,0
        {0x00, 0x00, 0x00}, //0,0,0
        {0x5F, 0x5F, 0x5F}, //255,255,255
        {0x00, 0x00, 0x00}, //0,0,0
        {0x00, 0x00, 0x00}, //0,0,0
        {0x5F, 0x5F, 0x5F}, //255,255,255
        {0x00, 0x00, 0x00}, //0,0,0
};

const led_ring_animation_t flag_ally = {
        &flag_ally_colors[0],
        9,
        DEFAULT_ANIM_SPEED,
        "Ally"
};

//////////// unlockable: //////////

const rgbcolor_t flag_leather_colors[] = {
        {0x08, 0x08, 0x6B}, //24,24,107
        {0x00, 0x00, 0x00}, //0,0,0
        {0x00, 0x00, 0x00}, //0,0,0
        {0x08, 0x08, 0x6B}, //24,24,107
        {0x08, 0x08, 0x6B}, //24,24,107
        {0xFF, 0xFF, 0xFF}, //255,255,255
        {0x08, 0x08, 0x6B}, //24,24,107
        {0x08, 0x08, 0x6B}, //24,24,107
        {0x00, 0x00, 0x00}, //0,0,0
        {0x00, 0x00, 0x00}, //0,0,0
        {0x00, 0x00, 0x00}, //0,0,0
        {0xFF, 0x00, 0x00}, //231,0,57
        {0x00, 0x00, 0x00}, //0,0,0
        {0x00, 0x00, 0x00}, //0,0,0
};

const led_ring_animation_t flag_leather = {
        &flag_leather_colors[0],
        14,
        DEFAULT_ANIM_SPEED,
        "Leather"
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

const led_ring_animation_t flag_bear = {
        &flag_bear_colors[0],
        8,
        DEFAULT_ANIM_SPEED,
        "Bear"
};

const rgbcolor_t flag_blue_colors[] = {
        //22, 13, 203
        {0x00, 0x00, 0xFF},
};

const led_ring_animation_t flag_blue = {
        &flag_blue_colors[0],
        1,
        DEFAULT_ANIM_SPEED,
        "Blue"
};

const rgbcolor_t flag_lblue_colors[] = {
        //153,235,255
        {0x00, 0xC0, 0xFF},
};

const led_ring_animation_t flag_lblue = {
        &flag_lblue_colors[0],
        1,
        DEFAULT_ANIM_SPEED,
        "Light blue"
};

const rgbcolor_t flag_green_colors[] = {
        //76,187,23
        {0x0F, 0xFF, 0x07},
};

const led_ring_animation_t flag_green = {
        &flag_green_colors[0],
        1,
        DEFAULT_ANIM_SPEED,
        "Green"
};

const rgbcolor_t flag_red_colors[] = {
        //255,0,0
        {0xFF, 0x00, 0x00},
};

const led_ring_animation_t flag_red = {
        &flag_red_colors[0],
        1,
        DEFAULT_ANIM_SPEED,
        "Red"
};

const rgbcolor_t flag_yellow_colors[] = {
        //255,255,0
        {0xE0, 0xA0, 0x00},
};

const led_ring_animation_t flag_yellow = {
        &flag_yellow_colors[0],
        1,
        DEFAULT_ANIM_SPEED,
        "Yellow"
};

const rgbcolor_t flag_pink_colors[] = {
        //249,162,241
        {0xd5, 0x00, 0x69},
};

const led_ring_animation_t flag_pink = {
        &flag_pink_colors[0],
        1,
        DEFAULT_ANIM_SPEED,
        "Pink"
};

/// end of flags //////

const led_ring_animation_t *flags[FLAG_COUNT] = {
        &flag_rainbow,
        &flag_bi,
        &flag_pan,
        &flag_trans,
        &flag_ace,
        &flag_ally,
        &flag_leather,
        &flag_bear,
        &flag_blue,
        &flag_lblue,
        &flag_green,
        &flag_red,
        &flag_yellow,
        &flag_pink,
};

///////////////////////

rgbcolor_t rainbow1[10] = {
        // rainbow colors
        { 0x0f, 0x00, 0x00 },
        { 0xff, 0x00, 0x00 },
        { 0x0f, 0x0f, 0x00 },
        { 0x00, 0xff, 0x00 },
        { 0x00, 0x0f, 0x0f },
        { 0x00, 0x00, 0xff },
        { 0x10, 0x10, 0x20 },
        { 0xe0, 0xe0, 0xe0 },
        { 0x10, 0x10, 0x10 },
        { 0x00, 0x00, 0x00 },
};

const rgbcolor_t test_colors[3] = {
        { 0xff, 0x00, 0x00},
        { 0x00, 0xff, 0x00 },
        { 0x00, 0x00, 0xff },
};
