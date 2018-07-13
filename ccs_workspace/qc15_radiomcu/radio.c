/*
 * radio.c
 *
 *  Created on: Jun 27, 2018
 *      Author: george
 */
#include <stdint.h>

#include <msp430.h>

#include "qc15.h"

#include "radio.h"
#include "rfm75.h"
#include "util.h"

// ugggh, this is SO wasteful. TODO: use a linked list.
uint8_t ids_in_range[QC15_BADGES_IN_SYSTEM+1] = {0};

uint8_t validate(radio_proto *msg, uint8_t len) {
    if (len != sizeof(radio_proto)) {
        // PROBLEM
        // TODO: check that this is working correctly, please.
        return 0;
    }

    // Check for bad ID:
    if (msg->badge_id > QC15_BADGES_IN_SYSTEM)
        return 0;

    // Finally, verify the CRC:
    return (crc16_check_buffer((uint8_t *) msg, len-2));

}

void radio_rx_done(uint8_t* data, uint8_t len, uint8_t pipe) {
    if (!validate((radio_proto *) data, len)) {
        // fail
        return;
    }

}

void radio_tx_done(uint8_t ack) {

}

void radio_init() {
    rfm75_init(35, &radio_rx_done, &radio_tx_done);
    rfm75_post();
}
