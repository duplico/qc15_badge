/*
 * util.c
 *
 *  Created on: Jun 17, 2018
 *      Author: george
 */

#include <msp430.h>
#include <driverlib.h>

#include "qc15.h"

void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(MCLK_FREQ_KHZ);
        mils--;
    }
}

/// Compute a 16-bit CRC on `len` bytes of byte buffer `buf`.
uint16_t crc16_compute(uint8_t *buf, uint16_t len) {
    CRC_setSeed(CRC_BASE, QC15_CRC_SEED);
    for (uint16_t i=0; i<len; i++) {
        CRC_set8BitData(CRC_BASE, buf[i]);
    }
    return CRC_getResult(CRC_BASE);
}

/// Append a 16-bit CRC onto the end of a `len`-byte buffer `buf`.
/**
 ** Note that `len` does NOT include the CRC.
 */
void crc16_append_buffer(uint8_t *buf, uint16_t len) {
    uint16_t crc = crc16_compute(buf, len);
    buf[len] = crc & 0xFF; // TODO: Check whether this matches MSP430 Endianness
    buf[len + 1] = (crc >> 8) & 0xFF;
}

/// Check whether a 16-bit CRC is at the end of a `len`-byte buffer `buf`.
/**
 ** Note that there must be room in the buffer AFTER len for the CRC.
 */
uint8_t crc16_check_buffer(uint8_t *buf, uint16_t len) {
    uint16_t crc = crc16_compute(buf, len);
    return (buf[len] == (crc & 0xFF)) && (buf[len+1] == ((crc >> 8) & 0xFF));
}
