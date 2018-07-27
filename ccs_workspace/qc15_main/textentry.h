/*
 * text_entry.h
 *
 *  Created on: Jul 23, 2018
 *      Author: george
 */

#ifndef TEXTENTRY_H_
#define TEXTENTRY_H_

void textentry_begin(char *destination, uint8_t len, uint8_t start_populated,
                     uint8_t show_instructions);
void textentry_handle_loop();


#endif /* TEXTENTRY_H_ */
