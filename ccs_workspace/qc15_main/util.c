/*
 * util.c
 *
 *  Created on: Jun 17, 2018
 *      Author: george
 */

void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(5000);
        mils--;
    }
}
