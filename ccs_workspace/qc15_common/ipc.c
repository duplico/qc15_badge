/*
 * ipc.c
 *
 *  Created on: Jul 7, 2018
 *      Author: george
 */

#include <stdint.h>

#include <msp430.h>
#include <driverlib.h>

void ipc_init() {
    // USCI A0 is our IPC UART:
    //  (on both chips! Yay!)

    // 8-bit data
    // No parity
    // 1 stop
    // (8N1)
    // LSB first
    // Baud Rate calculation:
    // 1000000/(9600) = 104.16667
    // OS16 = 1
    // UCBRx = INT(N/16) = 6.5104,
    // UCBRFx = INT([N/16 - INT(N/16)] x 16) = 8.1667
    // UCBRSx = 0x11 (per Table 22-4 but T22-5 says 0x20 will also work)

    // Register-based config:
    UCA0CTLW0 |= UCSWRST;
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BR0 = 6; // 1000000/9600/16
    UCA0BR1 = 0x00;
    UCA0MCTLW = 0x1100 | UCOS16 | UCBRF_8;
    UCA0CTLW0 &= ~UCSWRST;

}
