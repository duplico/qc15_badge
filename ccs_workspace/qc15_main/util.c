/*
 * util.c
 *
 *  Created on: Jun 17, 2018
 *      Author: george
 */

#define CLOCK_FREQ_KHZ 8000

void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(CLOCK_FREQ_KHZ);
        mils--;
    }
}
