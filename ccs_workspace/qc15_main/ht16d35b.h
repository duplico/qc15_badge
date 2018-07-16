/*
 * ht16d35b.h
 *
 *  Created on: Jun 17, 2018
 *      Author: george
 */

#ifndef HT16D35B_H_
#define HT16D35B_H_

#include <stdint.h>

/// The initial global brightness setting for the LED controller (0x40 is max).
#define HT16D_INITIAL_BRIGHTNESS 0x10

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgbcolor_t;

typedef struct {
    uint16_t r;
    uint16_t g;
    uint16_t b;
} rgbcolor16_t;

void ht16d_init_io();
void ht16d_init();
uint8_t ht16d_post();
void ht16d_send_gray();
void ht16d_all_one_color(uint8_t r, uint8_t g, uint8_t b);
void ht16d_all_one_color_ring_only(uint8_t r, uint8_t g, uint8_t b);
void ht16d_set_colors(uint8_t id_start, uint8_t id_end, rgbcolor16_t* colors);

#endif /* HT16D35B_H_ */
