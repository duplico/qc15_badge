/*
 * flash.h
 *
 *  Created on: Jun 15, 2018
 *      Author: george
 */

#ifndef S25FL_H_
#define S25FL_H_

void s25fl_init();
void s25fl_init_io();
uint8_t s25fl_post();
void s25fl_erase_block_64kb(uint32_t address);

void s25fl_read_data(uint8_t* buffer, uint32_t address, uint32_t len_bytes);
void s25fl_write_data(uint32_t address, uint8_t* buffer, uint8_t len_bytes);

#endif /* S25FL_H_ */
