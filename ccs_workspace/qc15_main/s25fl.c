/*
 * flash.c
 *
 *  Created on: Jun 15, 2018
 *      Author: george
 */

#include <driverlib.h>
#include <stdint.h>

#include "qc15.h"

const uint8_t FLASH_STATE_WREN  = BIT1;
const uint8_t FLASH_STATE_SLEEP = BIT2;
const uint8_t FLASH_STATE_BUSY  = BIT3;
const uint8_t FLASH_STATE_HOLD  = BIT4;

uint8_t flash_state = 0;

uint8_t flash_status_register = 0;

const uint8_t FLASH_SR_WIP = BIT1;

const uint8_t FLASH_CMD_WREN = 0x06;
const uint8_t FLASH_CMD_WRDIS = 0x04;
const uint8_t FLASH_CMD_READ_STATUS = 0x05;
const uint8_t FLASH_CMD_READ_ID = 0x9F;
const uint8_t FLASH_CMD_WRITE_STATUS = 0x01;
const uint8_t FLASH_CMD_READ_DATA = 0x03;
const uint8_t FLASH_CMD_PAGE_PROGRAM = 0x02;
const uint8_t FLASH_CMD_BLOCK_ERASE = 0xD8;
const uint8_t FLASH_CMD_SECTOR_ERASE = 0x20;
const uint8_t FLASH_CMD_CHIP_ERASE = 0xC7; // TODO: delay after these three:
const uint8_t FLASH_CMD_POWER_DOWN = 0xB9;
const uint8_t FLASH_CMD_POWER_UP = 0xAB;

#define FLASH_EUSCI_A_BASE EUSCI_A1_BASE

void s25fl_usci_a1_send_sync(uint8_t data) {
    while (!(UCA1IFG & UCTXIFG)); // wait for ready to accept a character.
    UCA1TXBUF = data;
    while (!(UCA1IFG & UCRXIFG)); // wait for completion, and reception.
    uint8_t dummy = UCA1RXBUF;// Throw away the stale garbage we got while sending.
}

uint8_t s25fl_usci_a1_recv_sync(uint8_t data) {
    while (!(UCA1IFG & UCTXIFG)); // Wait for ready to accept tx char
    UCA1TXBUF = data;
    while (!(UCA1IFG & UCRXIFG)); // wait for rx finish
    uint8_t retval = UCA1RXBUF;
    return retval;
}

void s25fl_begin() {
    // CS#      P3.6 (idle high)
    P3OUT &= ~BIT7; // CS low, select
}

void s25fl_end() {
    P3OUT |= BIT7; // CS high, deselect

}

void s25fl_simple_cmd(uint8_t cmd) {
    s25fl_begin();
    s25fl_usci_a1_send_sync(cmd);
    s25fl_end();
}

void s25fl_wr_en() {
    s25fl_simple_cmd(FLASH_CMD_WREN);
}

void s25fl_wr_dis() {
    s25fl_simple_cmd(FLASH_CMD_WRDIS);
}

uint32_t s25fl_rdid() {
    volatile uint32_t retval = 0;
    volatile uint32_t rxdat = 0;

    s25fl_begin();
    s25fl_usci_a1_send_sync(FLASH_CMD_READ_ID);
    rxdat = s25fl_usci_a1_recv_sync(0xff);
    retval |= (rxdat << 24);
    rxdat = s25fl_usci_a1_recv_sync(0xff);
    retval |= (rxdat << 16);
    rxdat = s25fl_usci_a1_recv_sync(0xff);
    retval |= (rxdat << 8);
//    rxdat = usci_a1_recv_sync(0xff);
//    retval |= rxdat;
    s25fl_end();
    return retval;
}

uint8_t s25fl_get_status() {
    s25fl_begin();
    s25fl_usci_a1_send_sync(FLASH_CMD_READ_STATUS);
    volatile uint8_t retval = s25fl_usci_a1_recv_sync(0xff);
    s25fl_end();
    flash_status_register = retval;
    return retval;
}

uint8_t s25fl_set_status(uint8_t status) {
    s25fl_begin();
    s25fl_usci_a1_send_sync(FLASH_CMD_WRITE_STATUS);
    s25fl_usci_a1_send_sync(status);
    s25fl_end();
    return s25fl_get_status() == status;
}

void s25fl_block_while_wip() {
    // Make sure nothing is in progress:
    while (s25fl_get_status() & FLASH_SR_WIP)
        __delay_cycles(100); // TODO: this number came from nowhere.
}

void s25fl_read_data(uint8_t* buffer, uint32_t address, uint32_t len_bytes) {
    s25fl_block_while_wip();
    s25fl_begin();
    s25fl_usci_a1_send_sync(FLASH_CMD_READ_DATA);
    s25fl_usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    s25fl_usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    s25fl_usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    // TODO: should be done with DMA, probably.
    for (uint32_t i = 0; i < len_bytes; i++) {
        buffer[i] = s25fl_usci_a1_recv_sync(0xff);
    }
    s25fl_end();
}

void s25fl_write_data(uint32_t address, uint8_t* buffer, uint32_t len_bytes) {
    // Length may not be any longer than 255.
    s25fl_block_while_wip();
    s25fl_begin();
    s25fl_usci_a1_send_sync(FLASH_CMD_PAGE_PROGRAM);
    s25fl_usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    s25fl_usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    s25fl_usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    // TODO: should be done with DMA, probably.
    for (uint32_t i = 0; i < len_bytes; i++) {
        s25fl_usci_a1_send_sync(buffer[i]);
    }
    s25fl_end();
}

void s25fl_erase_chip() {
    s25fl_block_while_wip();
    s25fl_simple_cmd(FLASH_CMD_CHIP_ERASE);
}

void s25fl_erase_block_64kb(uint32_t address) {
    s25fl_block_while_wip();
    s25fl_begin();
    s25fl_usci_a1_send_sync(FLASH_CMD_BLOCK_ERASE);
    s25fl_usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    s25fl_usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    s25fl_usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    s25fl_end();
}

void s25fl_erase_sector_4kb(uint32_t address) {
    s25fl_block_while_wip();
    s25fl_begin();
    s25fl_usci_a1_send_sync(FLASH_CMD_SECTOR_ERASE);
    s25fl_usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    s25fl_usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    s25fl_usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    s25fl_end();
}

void s25fl_sleep() {
    s25fl_block_while_wip();
    s25fl_simple_cmd(FLASH_CMD_POWER_DOWN);
}

void s25fl_wake() {
    s25fl_simple_cmd(FLASH_CMD_POWER_UP);
    // TODO: delay 3 us
    __delay_cycles(30);
}

/// Basic power-on self-test of serial flash, returning 1 on success.
uint8_t s25fl_post() {
    // TODO: Check the correct RDID on the flash:
    s25fl_rdid();

    volatile uint8_t status;
    volatile uint8_t initial_status = s25fl_get_status();

    s25fl_wr_en();
    status = s25fl_get_status();
    if (status != (initial_status | BIT1)) {
        return 0;
    }

    s25fl_wr_dis();
    status = s25fl_get_status();
    if (status != (initial_status & ~BIT1)) {
        return 0;
    }

    return 1;
}

/// Decouple our pins from the flash chip, in order to allow external control.
void s25fl_hold_io() {
    // Flash (2018):
    // CS#      P3.7 (idle high) x
    // HOLD#    P3.3 (idle high) x
    // WP#       J.3 (idle high) x
    // CLK      3.6 (A1)x
    // SOMI     3.5 (A1) x
    // SIMO     3.4 (A1) x

    // The Flashcat connects the following pins:
    // * CS#  (P3.7)
    // * CLK  (P3.6)
    // * SOMI (P3.5)
    // * MISO (P3.4)
    GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    // The others (HOLD# and WP#) are already set as output pins.
    // Let's make sure they're in a state where the programmer can write to
    //  the device:
    P3OUT |= BIT3; // Bring HOLD# high (device is held when HOLD# is low)
    PJOUT |= BIT3; // Bring WP# high (device write-protected when WP# is low)
}

void s25fl_init_io() {
    // Flash (2018):
    // CS#      P3.7 (idle high)
    // HOLD#    P3.3 (idle high)
    // WP#       J.3 (idle high)
    // CLK      3.6 (A1)
    // SOMI     3.5 (A1)
    // SIMO     3.4 (A1)

    // CS# high normally
    // HOLD# high normally
    // WP# high normally (write protect when low)

    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN3 + GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_PJ, GPIO_PIN3);
    P3OUT |= BIT7+BIT3; // Unheld, unselected
    PJOUT |= BIT3;  // unprotected.
    // TODO use preprocessor defines for the above.

    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P3, GPIO_PIN4 + GPIO_PIN5 + GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION);

    EUSCI_A_SPI_initMasterParam ucaparam = {0};
    ucaparam.clockPhase= EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
    ucaparam.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
    ucaparam.msbFirst = EUSCI_A_SPI_MSB_FIRST;
    ucaparam.spiMode = EUSCI_A_SPI_3PIN;
    ucaparam.selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK;
    ucaparam.clockSourceFrequency = SMCLK_FREQ_HZ;
    ucaparam.desiredSpiClock = 100000; // TODO: decide

    EUSCI_A_SPI_initMaster(EUSCI_A1_BASE, &ucaparam);
}

void s25fl_init() {
    // TODO: implement WP

    EUSCI_A_SPI_enable(EUSCI_A1_BASE);
}
