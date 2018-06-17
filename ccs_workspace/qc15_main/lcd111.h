/*
 * lcd111.h
 *
 *  Created on: Jun 17, 2018
 *      Author: george
 */

#ifndef LCD111_H_
#define LCD111_H_

void lcd111_init_io();
void lcd111_init();
void lcd111_text(uint8_t lcd_id, char *text);

#endif /* LCD111_H_ */
