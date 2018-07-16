/*
 * base_main.c
 *
 *  Created on: Jun 15, 2018
 *      Author: @Aradis
 */

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "driverlib.h"
#include <msp430fr2433.h>

#include "radio.h"
#include "rfm75.h"
#include "base_main.h"

void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(1000);
        mils--;
    }
}

void delay_nanos(unsigned long nanos) {
    while (nanos) {
        __delay_cycles(1);
        nanos--;
    }
}

void led_on() {
    P2OUT |= BIT2;
}

void led_off() {
    P2OUT &= ~BIT2;
}

void led_flash() {
    led_on();
    delay_millis(10);
    led_off();
}

void radio_tx_done(uint8_t ack) {

}

/**
 * Callback function when msg is received to format data into progress struct.
 */
void radio_rx_done(uint8_t* data, uint8_t len, uint8_t pipe) {
    // it was an rx:
    // light some shit up!
    led_flash();
    radio_progress_payload payload = create_payload(data[0], &data[1]);
    send_progress_payload(payload);
}

void init_io() {
    // The magic FRAM make-it-work command:
    PMM_unlockLPM5(); // PM5CTL0 &= ~LOCKLPM5;

    P2DIR |= BIT2; // LED
    P2OUT &= ~BIT2;

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
    UCA0MCTLW = 0x2200 | UCOS16 | UCBRF_13;
    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
}

/**
 * Sends a single character over serial.
 */
void send_char(char char_to_send) {
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, char_to_send);
    delay_nanos(170);
}

/**
 * Sends string over serial.
 */
void send_string(unsigned char * string_to_send, int length) {
    for (unsigned int i = length; i > 0; i--) {
        send_char(string_to_send[length-i]);
    }
}

/**
 * Transmits a progress payload over serial.
 */
void send_progress_payload(radio_progress_payload payload) {
    send_string(&payload.part_id, 1);
    send_char(',');
    send_string(payload.part_data, 10);
    send_char(0x0D);
    send_char(0x0A);
}

/**
 * Constructor for
 */
radio_progress_payload create_payload(uint8_t part_id, uint8_t part_data[10]) {
    radio_progress_payload payload;
    payload.part_id = part_id;
    memcpy(payload.part_data, part_data, 10);
    return payload;
}

void main (void) {
    WDTCTL = WDTPW | WDTHOLD; // Hold WDT

    // On boot, the clock system is as follows:
    // * MCLK and SMCLK -> DCOCLKDIV (divided DCO) (1 MHz)
    // * ACLK           -> REFO (32k internal oscillator)
    // char message[] = "HELLO WORLD";
    init_io();
    serial_init();

    rfm75_init(35, &radio_rx_done, &radio_tx_done);
    rfm75_post();
    __bis_SR_register(GIE);

    while (1) {
        if (f_rfm75_interrupt) {
            f_rfm75_interrupt = 0;
            rfm75_deferred_interrupt();
        }

        __bis_SR_register(LPM0_bits);
    }
 }
