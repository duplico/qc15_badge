#include <stdint.h>
#include <stdio.h>
#include "driverlib.h"
#include <msp430fr2433.h>

void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(1000);
        mils--;
    }
}

void init_io() {
    // The magic FRAM make-it-work command:
    PMM_unlockLPM5(); // PM5CTL0 &= ~LOCKLPM5;

    P1DIR |= BIT0; // LED
    P1OUT &= ~BIT0;

    // TX/RX
    P1SEL0 |= BIT4+BIT5;
    P1SEL1 &= ~(BIT4+BIT5); // unneeded but whatever.
}

void serial_init() {
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
    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI

    // Equivalent Driverlib config (with alternative UCBRSx of 0x20):

//    EUSCI_A_UART_initParam uart_param = {0};
//
//    uart_param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK; // 1 MHz
//    uart_param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION; // 1
//    uart_param.clockPrescalar = 6;
//    uart_param.firstModReg = 8;
//    uart_param.secondModReg = 0x20; // 1/6/8/0x20 = 9600 @ 1 MHz
//    uart_param.parity = EUSCI_A_UART_NO_PARITY;
//    uart_param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
//    uart_param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
//    uart_param.uartMode = EUSCI_A_UART_MODE;
//
//    EUSCI_A_UART_init(EUSCI_A0_BASE, &uart_param);
//    EUSCI_A_UART_enable(EUSCI_A0_BASE);
}

void send_char(char charToSend) {
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, charToSend);
    delay_millis(10);
}

void send_string(char * stringToSend, int length) {
    for (unsigned int i = length; i > 0; i--) {
        send_char(stringToSend[length-i]);
    }
}

void main (void) {
    WDTCTL = WDTPW | WDTHOLD; // Hold WDT

    // On boot, the clock system is as follows:
    // * MCLK and SMCLK -> DCOCLKDIV (divided DCO) (1 MHz)
    // * ACLK           -> REFO (32k internal oscillator)
    char message[] = "HELLO WORLD";
    init_io();
    serial_init();


    while (1) {
        unsigned int length = (int) (sizeof(message) / sizeof(message[0]));
        send_string(message, length);
        send_char(',');
    }
 }
