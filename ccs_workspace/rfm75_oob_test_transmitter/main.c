/*
 * main.c
 *
 *  Created on: Jun 15, 2018
 *      Author: @duplico
 */


/*
 * P1.0 radio CS - 1.0
 * P1.1 radio CLK - 1.5
 * P1.2 radio SIMO - 1.4
 * P1.3 radio SOMI - 1.6
 * P1.6 radio EN   - 2.5
 * P1.7 radio IRQ  - 2.6
 * P2.0 XTAL
 * P2.1 XTAL
 * P2.2 switch (right is high)
 *   |----/\/\/\/\/-----VCC
 */

#include <stdint.h>

#include "driverlib.h"
#include <msp430fr2433.h>

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

    P1DIR |= BIT1; // Green LED
    P1OUT &= ~BIT1;

//     * P2.5 radio EN
//     * P2.6 radio IRQ
}

void main (void)
{
    //Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);

    init_io();

    // On boot, the clock system is as follows:
    // * MCLK and SMCLK -> DCOCLKDIV (divided DCO) (1 MHz)
    // * ACLK           -> REFO (32k internal oscillator)

    rfm75_init();
    rfm75_post();

    __bis_SR_register(GIE);

    uint16_t millis = 0;

    while (1) {
        // check for attached radio.
        if (rfm75_post()) {
            rfm75_init();
            rfm75_tx();
            delay_millis(250);
        }

        if (f_rfm75_interrupt) {
            f_rfm75_interrupt = 0;
            rfm75_deferred_interrupt();
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
