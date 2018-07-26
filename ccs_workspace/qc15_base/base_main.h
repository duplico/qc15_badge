/*
 * base_main.h
 *
 *  Created on: Jul 15, 2018
 *      Author: @aradis
 */

#include "rfm75.h"
#include "radio.h"

#ifndef BASE_MAIN_H_
#define BASE_MAIN_H_

// Delay Functions
//void delay_millis(unsigned long mils);
void delay_nanos(unsigned long nanos);

// LED Functions
void led_on();
void led_off();
void led_flash();
void flash_p2_6();

// Radio Functions
void radio_tx_done(uint8_t ack);
void radio_rx_done(uint8_t* data, uint8_t len, uint8_t pipe);

// Init functions
void init_io();
void serial_init();
void timer_init();

// USB Functions
void send_char(char char_to_send);
void send_uint16_t(uint16_t int_to_send);
void send_string(unsigned char * string_to_send, uint8_t length);
void send_progress_payload(uint16_t badge_id, radio_progress_payload *payload);
void send_stats_payload(uint16_t badge_id, radio_stats_payload *payload);
void send_debug_payload(uint16_t badge_id, unsigned char* message);
void beacon();

void TIMER_ISR();

#endif /* BASE_MAIN_H_ */
