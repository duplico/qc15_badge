/*
 * main.c
 *
 *  Created on: Jun 15, 2018
 *      Author: @duplico
 */


/*
 * P1.0 radio CS
 * P1.1 radio CLK
 * P1.2 radio SIMO
 * P1.3 radio SOMI
 * P1.6 radio EN
 * P1.7 radio IRQ
 * P2.2 switch (right is high)
 *   |----/\/\/\/\/-----VCC
 */

#include <stdint.h>

#include "driverlib.h"
#include <msp430.h>

#include "rfm75.h"

uint16_t status;

void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(1000);
        mils--;
    }
}

void init_io() {
    // The magic FRAM make-it-work command:
    PMM_unlockLPM5();

    P2DIR |= BIT2;
    P2OUT &= ~BIT2;
}

void main (void)
{
    //Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);

    init_io();

    // On boot, the clock system is as follows:
    // * MCLK and SMCLK -> DCOCLKDIV (divided DCO) (1 MHz)
    // * ACLK           -> REFO (32k internal oscillator)

    rfm75_init(25);
    rfm75_post();

    __bis_SR_register(GIE);

    while (1) {
        // check for attached radio.
        if (rfm75_post()) {
            rfm75_init(25);
//            rfm75_tx(0xffff);
            rfm75_tx(35);
            delay_millis(200);
        }

        if (f_rfm75_interrupt) {
            f_rfm75_interrupt = 0;
            if (rfm75_deferred_interrupt() & 0b01) {
                P2OUT |= BIT2;
                delay_millis(50);
                P2OUT &= ~BIT2;
            }
        }
    }
}

#pragma vector=UNMI_VECTOR
__interrupt
void NMI_ISR(void)
{
  do {
    // If it still can't clear the oscillator fault flags after the timeout,
    // trap and wait here.
    status = CS_clearAllOscFlagsWithTimeout(1000);
  } while(status != 0);
}
