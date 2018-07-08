/*
 * radio.c
 *
 *  Created on: Jun 27, 2018
 *      Author: george
 */
#include "radio.h"
#include "rfm75.h"

void radio_rx_done(uint8_t* data, uint8_t len, uint8_t pipe) {

}

void radio_tx_done(uint8_t ack) {

}

void radio_init() {
    rfm75_init(35, &radio_rx_done, &radio_tx_done); // TODO: ID
    rfm75_post();
}
