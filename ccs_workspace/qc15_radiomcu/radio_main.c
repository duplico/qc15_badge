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
 * P1.4 IPC TX (A0TX)
 * P1.5 IPC RX (A0RX)
 * P1.6 radio EN
 * P1.7 radio IRQ
 * P2.0 XTAL
 * P2.1 XTAL
 * P2.2 switch (right is high)
 *   |----/\/\/\/\/-----VCC
 */

#include <stdint.h>

#include "driverlib.h"
#include <msp430fr2422.h>

#define CLOCK_FREQ_KHZ 1000

void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(CLOCK_FREQ_KHZ);
        mils--;
    }
}

void main (void)
{
    //Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);
    PMM_unlockLPM5();

    EUSCI_A_UART_initParam uart_param = {0};

    uart_param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK; // 1 MHz
    uart_param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION; // 1
    uart_param.clockPrescalar = 6;
    uart_param.firstModReg = 13;
    uart_param.secondModReg = 0x22; // 1/6/13/0x22 = 9600 @ 1.047576 MHz
    uart_param.parity = EUSCI_A_UART_NO_PARITY;
    uart_param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    uart_param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    uart_param.uartMode = EUSCI_A_UART_MODE;

    EUSCI_A_UART_init(EUSCI_A0_BASE, &uart_param);
    EUSCI_A_UART_enable(EUSCI_A0_BASE);

//    // IPC TX/RX GPIO
//    P1SEL0 &= ~(BIT4+BIT5);
//    P1SEL1 &= ~(BIT4+BIT5);
//
//    P1DIR |= BIT4;  // UCA0TXD
//    P1DIR &= ~BIT5; // UCA0RXD

    P1SEL0 |=  (BIT4+BIT5);
    P1SEL1 &= ~(BIT4+BIT5);

    volatile uint32_t sclk = CS_getSMCLK();
    __no_operation();

    volatile uint8_t tx_out = 0;

    while (1) {
        while (!(UCA0IFG & UCTXIFG)); // wait to TX
        UCA0TXBUF = tx_out;
        tx_out++;
        __no_operation();
        delay_millis(500);
    }
}
