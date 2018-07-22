/*
 * lcd111.h
 *
 *  Created on: Jun 17, 2018
 *      Author: george
 */

#ifndef LCD111_H_
#define LCD111_H_

#define LCD111_CMD_CLR 0x01
#define LCD111_CMD_CH 0x02 // Return HOME
#define LCD111_CMD_OSC 0x03

#define LCD111_CURSOR_INVERTING 0b100
#define LCD111_CURSOR_LINE 0b010
#define LCD111_CURSOR_BLINK 0b001
#define LCD111_CURSOR_NONE 0b000

#define LCD_TOP 1
#define LCD_BTM 0

void lcd111_init_io();
void lcd111_init();
void lcd111_cursor_type(uint8_t lcd_id, uint8_t cursor_type);
void lcd111_cursor_pos(uint8_t lcd_id, uint8_t pos);
void lcd111_clear(uint8_t lcd_id);
void lcd111_put_char(uint8_t lcd_id, char character);
void lcd111_put_text(uint8_t lcd_id, char *text, uint8_t len);
void lcd111_set_text(uint8_t lcd_id, char *text);

#endif /* LCD111_H_ */
