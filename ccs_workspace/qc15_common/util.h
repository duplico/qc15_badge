/*
 * util.h
 *
 *  Created on: Jun 17, 2018
 *      Author: george
 */

#ifndef UTIL_H_
#define UTIL_H_

void delay_millis(unsigned long mils);

uint16_t crc16(uint8_t *buf, uint16_t len);
void crc16_append_buffer(uint8_t *buf, uint16_t len);
uint8_t crc16_check_buffer(uint8_t *buf, uint16_t len);

#endif /* UTIL_H_ */
