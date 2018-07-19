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

#define RADIO_STATS_MSG_LEN 35

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

    radio_progress_payload *progress_payload;
    radio_stats_payload *stats_payload;

    radio_proto *radio_msg = (radio_proto *) data;
    switch (radio_msg->msg_type) {
    case RADIO_MSG_TYPE_PROGRESS :
        progress_payload = (radio_progress_payload *) radio_msg->msg_payload;
//        send_progress_payload(radio_msg->badge_id, progress_payload);
        break;
    case RADIO_MSG_TYPE_STATS :
        stats_payload = (radio_stats_payload *) (radio_msg->msg_payload);
        send_stats_payload(radio_msg->badge_id, stats_payload);
        break;
    default :
//        send_debug_payload(radio_msg.badge_id, radio_msg.msg_payload);
        delay_nanos(5);
    }
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
}

void send_uint16_t(uint16_t int_to_send) {
    char long_string[2];
    sprintf(long_string, "%d", int_to_send);
    for (int i = 2; i > 0; i--) {
        send_char(long_string[2-i]);
    }
}

/**
 * Sends string over serial.
 */
void send_string(unsigned char * string_to_send, uint8_t length) {
    for (int i = length; i > 0; i--) {
        send_char(string_to_send[length-i]);
    }
}

/**
 * Transmits a progress payload over serial.
 */
void send_progress_payload(uint16_t badge_id, radio_progress_payload payload) {
    /**
     * Format:
     *
     * 3,badge_id,part_id,part_data(10 bytes long)\CR\LF
     *
     * part_data is rendered as as an unquoted 10-byte string with hex values escaped with `\x`.
     * For example: `\x0A\x1B\x34\x00\x01\x34\x01\x10\xA5\x23`
     */
    send_char(3);
    send_string(&payload.part_id, 1);
    send_char(',');
    send_string(payload.part_data, 10);
    // Carriage Return
    send_char(0x0D);
    // Line Feed
    send_char(0x0A);
}

void send_stats_payload(uint16_t badge_id, radio_stats_payload *payload) {
    // TODO: Test function
    /**
     * 4,badge_id,badges_seen_count,badges_connected_count,badges_uploaded_count,
     * ubers_seen_count,ubers_connected_count,ubers_uploaded_count,handlers_seen,
     * handlers_connected,handlers_uploaded_count\CR\LF
     */
    // 12+2+1+10 = 25 bytes
    uint8_t message[RADIO_STATS_MSG_LEN] = {0};
    sprintf(message, "4,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
            badge_id,
            payload->badges_seen_count,
            payload->badges_connected_count,
            payload->badges_uploaded_count,
            payload->ubers_seen_count,
            payload->ubers_connected_count,
            payload->ubers_uploaded_count,
            payload->handlers_seen,
            payload->handlers_connected,
            payload->handlers_uploaded_count);
    send_string(message, RADIO_STATS_MSG_LEN);
    // CRLF
    send_char(0x0D);
    send_char(0x0A);
}

void send_debug_payload(uint16_t badge_id, unsigned char* message) {
    // TODO: Implement.

}

void main (void) {
    WDTCTL = WDTPW | WDTHOLD; // Hold WDT

    // On boot, the clock system is as follows:
    // * MCLK and SMCLK -> DCOCLKDIV (divided DCO) (1 MHz)
    // * ACLK           -> REFO (32k internal oscillator)
    // char message[] = "HELLO WORLD";
    init_io();
    serial_init();

//    rfm75_init(35, &radio_rx_done, &radio_tx_done);
//    rfm75_post();
//    __bis_SR_register(GIE);

    radio_stats_payload stats = {0};
    stats.badges_seen_count = 0x00AA;
    stats.badges_connected_count = 0x00BB;
    stats.badges_uploaded_count = 0x00CC;
    stats.ubers_seen_count = 0x0D;
    stats.ubers_connected_count = 0x0E;
    stats.ubers_uploaded_count = 0x01;
    stats.handlers_seen = 0x02;
    stats.handlers_connected = 0x03;
    stats.handlers_uploaded_count = 0x04;

    uint16_t badge_id = 0x00AF;

    while (1) {
//        if (f_rfm75_interrupt) {
//            f_rfm75_interrupt = 0;
//            rfm75_deferred_interrupt();
//        }

//        __bis_SR_register(LPM0_bits);
        send_stats_payload(badge_id, &stats);
//        delay_millis(250);
    }
 }
