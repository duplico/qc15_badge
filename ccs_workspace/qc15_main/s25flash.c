/*
 * flash.c
 *
 *  Created on: Jun 15, 2018
 *      Author: george
 */

#include <driverlib.h>
#include <stdint.h>

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

void usci_a1_send_sync(uint8_t data) {
    // If we're held, wait:
    // TODO: don't busy wait.
    while (flash_state & FLASH_STATE_HOLD);
    while (!(UCA1IFG & UCTXIFG)); // wait for ready to accept a character.
    UCA1TXBUF = data;
    while (!(UCA1IFG & UCRXIFG)); // wait for completion, and reception.
    uint8_t dummy = UCA1RXBUF;// Throw away the stale garbage we got while sending.
}

uint8_t usci_a1_recv_sync(uint8_t data) {
    // If we're held, wait:
    // TODO: don't busy wait.
    while (flash_state & FLASH_STATE_HOLD);
    while (!(UCA1IFG & UCTXIFG)); // Wait for ready to accept tx char
    UCA1TXBUF = data;
    while (!(UCA1IFG & UCRXIFG)); // wait for rx finish
    uint8_t retval = UCA1RXBUF;
    return retval;
}

void flash_begin() {
    // If we're held, wait:
    // TODO: don't busy wait.
    while (flash_state & FLASH_STATE_HOLD);
    // CS#      P3.6 (idle high)
    P3OUT &= ~BIT7; // CS low, select
}

void flash_end() {
    // TODO:
    while (flash_state & FLASH_STATE_HOLD);
    P3OUT |= BIT7; // CS high, deselect

}

void flash_simple_cmd(uint8_t cmd) {
    flash_begin();
    usci_a1_send_sync(cmd);
    flash_end();
}

void flash_wr_en() {
    flash_simple_cmd(FLASH_CMD_WREN);
}

void flash_wr_dis() {
    flash_simple_cmd(FLASH_CMD_WRDIS);
}

uint32_t flash_rdid() {
    volatile uint32_t retval = 0;
    volatile uint32_t rxdat = 0;

    flash_begin();
    usci_a1_send_sync(FLASH_CMD_READ_ID);
    rxdat = usci_a1_recv_sync(0xff);
    retval |= (rxdat << 24);
    rxdat = usci_a1_recv_sync(0xff);
    retval |= (rxdat << 16);
    rxdat = usci_a1_recv_sync(0xff);
    retval |= (rxdat << 8);
//    rxdat = usci_a1_recv_sync(0xff);
//    retval |= rxdat;
    flash_end();
    return retval;
}

uint8_t flash_get_status() {
    flash_begin();
    usci_a1_send_sync(FLASH_CMD_READ_STATUS);
    volatile uint8_t retval = usci_a1_recv_sync(0xff);
    flash_end();
    flash_status_register = retval;
    return retval;
}

uint8_t flash_set_status(uint8_t status) {
    flash_begin();
    usci_a1_send_sync(FLASH_CMD_WRITE_STATUS);
    usci_a1_send_sync(status);
    flash_end();
    return flash_get_status() == status;
}

void flash_block_while_wip() {
    // Make sure nothing is in progress:
    while (flash_get_status() & FLASH_SR_WIP)
        __delay_cycles(100); // TODO: this number came from nowhere.
}

void flash_read_data(uint32_t address, uint32_t len_bytes, uint8_t* buffer) {
    flash_block_while_wip();
    flash_begin();
    usci_a1_send_sync(FLASH_CMD_READ_DATA);
    usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    // TODO: should be done with DMA, probably.
    for (uint32_t i = 0; i < len_bytes; i++) {
        buffer[i] = usci_a1_recv_sync(0xff);
    }
    flash_end();
}

void flash_write_data(uint32_t address, uint8_t len_bytes, uint8_t* buffer) {
    // Length may not be any longer than 255.
    flash_block_while_wip();
    flash_begin();
    usci_a1_send_sync(FLASH_CMD_PAGE_PROGRAM);
    usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    // TODO: should be done with DMA, probably.
    for (uint8_t i = 0; i < len_bytes; i++) {
        usci_a1_send_sync(buffer[i]);
    }
    flash_end();
}

void flash_erase_chip() {
    flash_block_while_wip();
    flash_simple_cmd(FLASH_CMD_CHIP_ERASE);
}

void flash_erase_block_64kb(uint32_t address) {
    flash_block_while_wip();
    flash_begin();
    usci_a1_send_sync(FLASH_CMD_BLOCK_ERASE);
    usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    flash_end();
}

void flash_erase_sector_4kb(uint32_t address) {
    flash_block_while_wip();
    flash_begin();
    usci_a1_send_sync(FLASH_CMD_SECTOR_ERASE);
    usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    flash_end();
}

void flash_sleep() {
    flash_block_while_wip();
    flash_simple_cmd(FLASH_CMD_POWER_DOWN);
}

void flash_wake() {
    flash_simple_cmd(FLASH_CMD_POWER_UP);
    // TODO: delay 3 us
    __delay_cycles(30);
}

/// Basic power-on self-test of serial flash, returning 1 on success.
uint8_t s25flash_post() {
    // TODO: Check the correct RDID on the flash:
    flash_rdid();

    volatile uint8_t status;
    volatile uint8_t initial_status = flash_get_status();

    flash_wr_en();
    status = flash_get_status();
    if (status != (initial_status | BIT1)) {
        return 0;
    }

    flash_wr_dis();
    status = flash_get_status();
    if (status != (initial_status & ~BIT1)) {
        return 0;
    }

    return 1;
}

// TODO: Add the ability to turn this off for external control.

void s25flash_init_io() {
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
    ucaparam.clockSourceFrequency = CS_getSMCLK();
    ucaparam.desiredSpiClock = 100000; // TODO

    EUSCI_A_SPI_initMaster(EUSCI_A1_BASE, &ucaparam);
}

void s25flash_init() {
    // Flash: (OLD)
    // CS#          P1.1
    // HOLD#   P1.0
    // WP#          P3.0

    // Flash (2018):
    // CS#      P3.6 (idle high, low to select)
    // HOLD#    P3.3 (idle high, low to hold)
    // WP#       J.3 (idle high, low to protect)
    // CLK      3.6 (A1)
    // SOMI     3.5 (A1)
    // SIMO     3.4 (A1)

    // TODO: implement WP

    EUSCI_A_SPI_enable(EUSCI_A1_BASE);
}
