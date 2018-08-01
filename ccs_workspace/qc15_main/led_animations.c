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
//        {0x5300, 0x1f00, 0x6600}, // 115, 79, 150
        {0,0,0},
        {0x50, 0x00, 0xff}, // 0, 56, 168
};
const rgbcolor_t flag_pan_colors[] = {
        {0xff, 0x21, 0x8c}, // 255,33,140
        {0xff, 0xd8, 0x00}, //255,216,0
        {0xff, 0xd8, 0x00}, //255,216,0
        {0x21, 0xb1, 0xff}, //33,177,255
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
        //22, 13, 203
        {0x00, 0x00, 0xFF},
};
const rgbcolor_t flag_lblue_colors[] = {
        //153,235,255
        {0x00, 0xC0, 0xFF},
};
const rgbcolor_t flag_green_colors[] = {
        //76,187,23
        {0x0F, 0xFF, 0x07},
};

const rgbcolor_t flag_red_colors[] = {
        //255,0,0
        {0xFF, 0x00, 0x00},
};
const rgbcolor_t flag_yellow_colors[] = {
        //255,255,0
        {0xE0, 0xA0, 0x00},
};

const rgbcolor_t flag_pink_colors[] = {
        //249,162,241
        {0xd5, 0x00, 0x69},
};

/////////////////////////////////

const rgbcolor_t rainbow_colors[] = {
        {255, 0, 0}, // Red
        {255, 24, 0x00}, // Orange
        {128, 40, 0x00}, // Yellow
        {0, 64, 0}, // Green
        {0, 0, 196}, // Blue
        {128, 0, 128}, // Purple
};

const led_ring_animation_t anim_rainbow = {
        &rainbow_colors[0],
        6,
        5,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SPIN,
};

const rgbcolor_t lsw_colors[] = {
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

//const led_ring_animation_t anim_lsw = { // "light solid white" (bright)
//        &lsw_colors[0],
//        12,
//        2,
//        HT16D_BRIGHTNESS_MAX,
//        LED_ANIM_TYPE_SPIN,
//        "solidwhite"
//};

const rgbcolor_t lwf_colors[] = { // "light white fade" (normal)
        {96, 96, 96},
        {255, 255, 255},
        {0, 0, 0},
        {96, 96, 96},
        {255, 255, 255},
        {0, 0, 0},
        {96, 96, 96},
        {255, 255, 255},
        {0, 0, 0},
};

//const led_ring_animation_t anim_lwf = {
//        &lwf_colors[0],
//        9,
//        16,
//        HT16D_BRIGHTNESS_DEFAULT,
//        LED_ANIM_TYPE_FALL,
//        "whitefall"
//};

const rgbcolor_t spinblue_colors[] = { // "light white fade" (normal)
        {0, 0, 10},
        {0, 0, 32},
        {0, 0, 64},
        {0, 0, 100},
        {0, 0, 196},
        {0, 0, 255},
};

//const led_ring_animation_t anim_spinblue = {
//        &spinblue_colors[0],
//        6,
//        6,
//        HT16D_BRIGHTNESS_DEFAULT,
//        LED_ANIM_TYPE_SPIN,
//        "bluespin"
//};

const led_ring_animation_t anim_fallblue = {
        &spinblue_colors[0],
        6,
        6,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_FALL,
        "fallblue"
};

const led_ring_animation_t anim_solidblue = {
        &spinblue_colors[0],
        6,
        6,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SAME,
        "solidblue"
};

const rgbcolor_t whitediscovery_colors[] = { // "light white fade" (normal)
        {255, 255, 255},
        {0, 0, 0},
        {64, 64, 64},
};

const led_ring_animation_t anim_whitediscovery = {
        &whitediscovery_colors[0],
        3,
        6,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SAME,
        "whitedisc"
};

const rgbcolor_t spinorange_colors[] = { // "light white fade" (normal)
        {255, 24, 0},
};

const led_ring_animation_t anim_spinorange = (led_ring_animation_t) {
        .colors = &spinorange_colors[0],
        .len = 1,
        .speed = 2,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
        .name = "sporange"
};

const rgbcolor_t solidgreen_colors[] = { // "light white fade" (normal)
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

const led_ring_animation_t anim_solidgreen = (led_ring_animation_t) {
        .colors = &solidgreen_colors[0],
        .len = 1,
        .speed = 1,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
        .name = "solidgn"
};

const rgbcolor_t solidyellow_colors[] = { // "light white fade" (normal)
        {128, 40, 0},
};

const led_ring_animation_t anim_solidyellow = (led_ring_animation_t) {
        .colors = &solidyellow_colors[0],
        .len = 1,
        .speed = 1,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
        .name = "solidylw"
};

const led_ring_animation_t anim_spinyellow = (led_ring_animation_t) {
        .colors = &solidyellow_colors[0],
        .len = 1,
        .speed = 1,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
        .name = "spinylw"
};

const led_ring_animation_t anim_fallyellow = (led_ring_animation_t) {
        .colors = &solidyellow_colors[0],
        .len = 1,
        .speed = 1,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_FALL,
        .name = "fallylw"
};

const led_ring_animation_t anim_spingreen = (led_ring_animation_t) {
        .colors = &solidgreen_colors[0],
        .len = 1,
        .speed = 1,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
        .name = "spingn"
};

const rgbcolor_t solidred_colors[] = { // "light white fade" (normal)
        {255, 0, 0},
};

const led_ring_animation_t anim_spinred = (led_ring_animation_t) {
        .colors = &solidred_colors[0],
        .len = 1,
        .speed = 1,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
        .name = "spinrd"
};

const led_ring_animation_t anim_solidorange = (led_ring_animation_t) {
        .colors = &spinorange_colors[0],
        .len = 1,
        .speed = 1,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
        .name = "solidorng"
};

const led_ring_animation_t anim_solidred = (led_ring_animation_t) {
        .colors = &solidred_colors[0],
        .len = 1,
        .speed = 1,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
        .name = "solidred"
};

const rgbcolor_t solidwhite_colors[] = { // "light white fade" (normal)
        {255, 255, 255},
};

const led_ring_animation_t anim_spinwhite = (led_ring_animation_t) {
        .colors = &solidwhite_colors[0],
        .len = 1,
        .speed = 1,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
        .name = "solidwhite"
};

const rgbcolor_t solidpink_colors[] = { // "light white fade" (normal)
        {128, 0, 128},
};

const led_ring_animation_t anim_spinpink = (led_ring_animation_t) {
        .colors = &solidpink_colors[0],
        .len = 1,
        .speed = 1,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
        .name="spinpink"
};


const rgbcolor_t pan_colors[] = {
        {0xff, 0x21, 0x8c}, // 255,33,140
        {0xff, 0xd8, 0x00}, //255,216,0
        {0xff, 0xd8, 0x00}, //255,216,0
        {0x21, 0xb1, 0xff}, //33,177,255
};

const led_ring_animation_t anim_pan = {
        &pan_colors[0],
        4,
        6,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SPIN,
        .name="pan"
};

const led_ring_animation_t all_animations[GAME_ANIMS_LEN+FLAG_COUNT] = {
    { // "light solid white" (bright)
      &lsw_colors[0],
      12,
      32,
      HT16D_BRIGHTNESS_MAX,
      LED_ANIM_TYPE_SAME,
      "solidwhite"
    },
    {
     &lwf_colors[0],
     9,
     16,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "whitefall"
    },
    {
     &spinblue_colors[0],
     6,
     6,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_SPIN,
     "bluespin"
    },
    {
     &whitediscovery_colors[0],
     3,
     6,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_SAME,
     "whitedisc"
    },
    {
     &spinblue_colors[0],
     6,
     32,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_SAME,
     "solidblue"
    },
    (led_ring_animation_t) {
        .colors = &spinorange_colors[0],
                .len = 1,
                .speed = 1,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_SPIN,
                .name = "sporange"
    },
    (led_ring_animation_t) {
        .colors = &solidgreen_colors[0],
                .len = 1,
                .speed = 32,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_SAME,
                .name = "solidgn"
    },
    (led_ring_animation_t) {
        .colors = &solidyellow_colors[0],
                .len = 1,
                .speed = 32,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_SAME,
                .name = "solidylw"
    },
    (led_ring_animation_t) {
        .colors = &solidyellow_colors[0],
                .len = 1,
                .speed = 1,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_SPIN,
                .name = "spinylw"
    },
    (led_ring_animation_t) {
        .colors = &solidgreen_colors[0],
                .len = 11,
                .speed = 2,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_SPIN,
                .name = "spingn"
    },
    (led_ring_animation_t) {
        .colors = &solidred_colors[0],
                .len = 1,
                .speed = 1,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_SPIN,
                .name = "spinrd"
    },
    (led_ring_animation_t) {
        .colors = &spinorange_colors[0],
                .len = 1,
                .speed = 32,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_SAME,
                .name = "solidorng"
    },
    (led_ring_animation_t) {
        .colors = &solidred_colors[0],
                .len = 1,
                .speed = 32,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_SAME,
                .name = "solidred"
    },
    (led_ring_animation_t) {
        .colors = &solidwhite_colors[0],
                .len = 1,
                .speed = 32,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_SPIN,
                .name = "solidwhite"
    },
    (led_ring_animation_t) {
        .colors = &solidpink_colors[0],
                .len = 1,
                .speed = 1,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_SPIN,
                .name="spinpink"
    },
    {
     &spinblue_colors[0],
     6,
     6,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "fallblue"
    },
    (led_ring_animation_t) {
        .colors = &solidyellow_colors[0],
                .len = 1,
                .speed = 1,
                .brightness = HT16D_BRIGHTNESS_DEFAULT,
                .type = LED_ANIM_TYPE_FALL,
                .name = "fallylw"
    },
    {
     &flag_rainbow_colors[0],
     6,
     DEFAULT_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Rainbow"
    },
    {
     &flag_bi_colors[0],
     4,
     DEFAULT_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Bisexual"
    },
    {
     &flag_pan_colors[0],
     4,
     DEFAULT_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Pansexual"
    },
    {
     &flag_trans_colors[0],
     8,
     DEFAULT_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Trans",
    },
    {
     &flag_ace_colors[0],
     9,
     DEFAULT_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Asexual",
    },
    {
     &flag_ally_colors[0],
     9,
     DEFAULT_ANIM_SPEED,
     HT16D_BRIGHTNESS_DEFAULT,
     LED_ANIM_TYPE_FALL,
     "Ally",
    },
     {
      &flag_leather_colors[0],
      14,
      DEFAULT_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Leather"
     },
     {
      &flag_bear_colors[0],
      8,
      DEFAULT_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Bear"
     },
     {
      &flag_blue_colors[0],
      1,
      DEFAULT_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Blue"
     },
     {
      &flag_lblue_colors[0],
      1,
      DEFAULT_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Light blue"
     },
     {
      &flag_green_colors[0],
      1,
      DEFAULT_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Green"
     },
     {
      &flag_red_colors[0],
      1,
      DEFAULT_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Red"
     },
     {
      &flag_yellow_colors[0],
      1,
      DEFAULT_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Yellow"
     },
     {
      &flag_pink_colors[0],
      1,
      DEFAULT_ANIM_SPEED,
      HT16D_BRIGHTNESS_DEFAULT,
      LED_ANIM_TYPE_FALL,
      "Pink"
     },
    };

//
//all_animations[0] = anim_lsw;
//all_animations[1] = anim_lwf;
//all_animations[2] = anim_spinblue;
//all_animations[3] = anim_whitediscovery;
//all_animations[4] = anim_solidblue;
//all_animations[5] = anim_spinorange;
//all_animations[6] = anim_solidgreen;
//all_animations[7] = anim_solidyellow;
//all_animations[8] = anim_spinyellow;
//all_animations[9] = anim_spingreen;
//all_animations[10] = anim_spinred;
//all_animations[11] = anim_solidorange;
//all_animations[12] = anim_solidred;
//all_animations[13] = anim_spinwhite;
//all_animations[14] = anim_spinpink;
//all_animations[15] = anim_fallblue;
//all_animations[16] = anim_fallyellow;


/////////////////////////////////////////////////////////////
