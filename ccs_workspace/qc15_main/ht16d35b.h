/*
 * ht16d35b.h
 *
 *  Created on: Jun 17, 2018
 *      Author: george
 */

#ifndef HT16D35B_H_
#define HT16D35B_H_

void ht16d_init_io();
void ht16d_init();
void led_send_gray();
void led_all_one_color(uint8_t r, uint8_t g, uint8_t b);
void led_all_one_color_ring_only(uint8_t r, uint8_t g, uint8_t b);

#endif /* HT16D35B_H_ */
