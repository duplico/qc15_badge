/// A minimal driver for the S25FS064S SPI flash module.
/**
 ** Gosh, I hope this works long enough for the conference weekend. These
 ** modules are accidentally 1.8V and we're running them at 3.3V...
 **
 ** \file s25fs.c
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#include <driverlib.h>
#include <stdint.h>

#include "qc15.h"
#include "flash_layout.h"

uint8_t flash_status_register = 0;

const uint8_t FLASH_SR_WIP = BIT0;

#define FLASH_CMD_WREN 0x06
#define FLASH_CMD_WRDIS 0x04
#define FLASH_CMD_READ_STATUS 0x05
#define FLASH_CMD_READ_ID 0x9F
#define FLASH_CMD_READ_DATA 0x03
#define FLASH_CMD_PAGE_PROGRAM 0x02
#define FLASH_CMD_ERASE_BLOCK 0xD8 // 64k blocks
#define FLASH_CMD_ERASE_PARAM_SECTOR 0x20 // These are the 4k things
#define FLASH_CMD_WRAR 0x71 // write any register
#define FLASH_CMD_RDAR 0x65 // read any register

#define FLASH_CMD_CHIP_ERASE 0xC7 // TODO: delay after these three:
#define FLASH_CMD_POWER_DOWN 0xB9
#define FLASH_CMD_POWER_UP 0xAB
#define FLASH_CMD_CLSR 0x82

#define FLASH_REG_ADDR_SR1V 0x800000
#define FLASH_REG_ADDR_SR2V 0x800001
#define FLASH_REG_ADDR_CR1V 0x800002 // p90 of data sheet
#define FLASH_REG_ADDR_CR2V 0x800003
#define FLASH_REG_ADDR_CR3V 0x800004
#define FLASH_REG_ADDR_CR4V 0x800005

#define FLASH_RDID_VAL_S25FS064S 0x0102174D

#define FLASH_EUSCI_A_BASE EUSCI_A1_BASE

void s25fs_usci_a1_send_sync(uint8_t data) {
    while (!(UCA1IFG & UCTXIFG)); // wait for ready to accept a character.
    UCA1TXBUF = data;
    while (!(UCA1IFG & UCRXIFG)); // wait for completion, and reception.
    uint8_t dummy = UCA1RXBUF;// Throw away the stale garbage we got while sending.
}

uint8_t s25fs_usci_a1_recv_sync(uint8_t data) {
    while (!(UCA1IFG & UCTXIFG)); // Wait for ready to accept tx char
    UCA1TXBUF = data;
    while (!(UCA1IFG & UCRXIFG)); // wait for rx finish
    uint8_t retval = UCA1RXBUF;
    return retval;
}

void s25fs_begin() {
    P3OUT &= ~BIT7; // CS low, select
}

void s25fs_end() {
    P3OUT |= BIT7; // CS high, deselect

}

void s25fs_simple_cmd(uint8_t cmd) {
    s25fs_begin();
    s25fs_usci_a1_send_sync(cmd);
    s25fs_end();
}

void s25fs_wr_en() {
    s25fs_simple_cmd(FLASH_CMD_WREN);
}

void s25fs_wr_dis() {
    s25fs_simple_cmd(FLASH_CMD_WRDIS);
}

uint32_t s25fs_rdid() {
    volatile uint32_t retval = 0;
    volatile uint32_t rxdat = 0;

    s25fs_begin();
    s25fs_usci_a1_send_sync(FLASH_CMD_READ_ID);
    rxdat = s25fs_usci_a1_recv_sync(0xff);
    retval |= (rxdat << 24);
    rxdat = s25fs_usci_a1_recv_sync(0xff);
    retval |= (rxdat << 16);
    rxdat = s25fs_usci_a1_recv_sync(0xff);
    retval |= (rxdat << 8);
    rxdat = s25fs_usci_a1_recv_sync(0xff);
    retval |= rxdat;
    s25fs_end();
    return retval;
}

uint8_t s25fs_get_status() {
    s25fs_begin();
    s25fs_usci_a1_send_sync(FLASH_CMD_READ_STATUS);
    volatile uint8_t retval = s25fs_usci_a1_recv_sync(0xff);
    s25fs_end();
    flash_status_register = retval;
    return retval;
}

uint8_t s25fs_read_any_register(uint32_t addr) {
    s25fs_begin();
    s25fs_usci_a1_send_sync(FLASH_CMD_RDAR);
    /// THREE BYTES of address: (ALL are MSByte first)
    s25fs_usci_a1_send_sync((addr & 0xff0000) >> 16);
    s25fs_usci_a1_send_sync((addr & 0x00ff00) >> 8);
    s25fs_usci_a1_send_sync((addr & 0x0000ff));

    // try a dummy:
    volatile uint8_t retval = s25fs_usci_a1_recv_sync(0xff);
    retval = s25fs_usci_a1_recv_sync(0xff);
    retval = s25fs_usci_a1_recv_sync(0xff);
    s25fs_end();
    return retval;
}

void s25fs_block_while_wip() {
    // Make sure nothing is in progress:
    // TODO: check & 0b01100000
    while (s25fs_get_status() & FLASH_SR_WIP)
        __delay_cycles(100); // TODO: this number came from nowhere.
}

void s25fs_wr_register(uint32_t addr, uint8_t val) {
    s25fs_block_while_wip();
    s25fs_begin();
    s25fs_usci_a1_send_sync(FLASH_CMD_WRAR);
    s25fs_usci_a1_send_sync((addr & 0x00FF0000) >> 16); // MSByte of address
    s25fs_usci_a1_send_sync((addr & 0x0000FF00) >> 8); // Middle byte of address
    s25fs_usci_a1_send_sync((addr & 0x000000FF)); // LSByte of address

    s25fs_usci_a1_send_sync(val);
    s25fs_end();
}

void s25fs_read_data(uint8_t* buffer, uint32_t address, uint32_t len_bytes) {
    s25fs_block_while_wip();
    s25fs_begin();
    s25fs_usci_a1_send_sync(FLASH_CMD_READ_DATA);
    s25fs_usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    s25fs_usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    s25fs_usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    for (uint32_t i = 0; i < len_bytes; i++) {
        buffer[i] = s25fs_usci_a1_recv_sync(0xff);
    }
    s25fs_end();
}

// TODO: Writes and erases are not working for some reason.
//
void s25fs_write_data(uint32_t address, uint8_t* buffer, uint32_t len_bytes) {
    // Length may not be any longer than 255.
    s25fs_block_while_wip();
    s25fs_begin();
    s25fs_usci_a1_send_sync(FLASH_CMD_PAGE_PROGRAM);
    s25fs_usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    s25fs_usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    s25fs_usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    for (uint32_t i = 0; i < len_bytes; i++) {
        s25fs_usci_a1_send_sync(buffer[i]);
    }
    s25fs_end();
}

void s25fs_erase_chip() {
    s25fs_block_while_wip();
    s25fs_simple_cmd(FLASH_CMD_CHIP_ERASE);
}

// TODO: Confirm erase address. Currently it's erroring.
void s25fs_erase_block_64kb(uint32_t address) {
    s25fs_block_while_wip();
    s25fs_begin();
    s25fs_usci_a1_send_sync(FLASH_CMD_ERASE_BLOCK);
    s25fs_usci_a1_send_sync((address & 0x00FF0000) >> 16); // MSByte of address
    s25fs_usci_a1_send_sync((address & 0x0000FF00) >> 8); // Middle byte of address
    s25fs_usci_a1_send_sync((address & 0x000000FF)); // LSByte of address
    s25fs_end();

    volatile uint8_t i = s25fs_get_status();
    __no_operation();
}

void s25fs_sleep() {
    s25fs_block_while_wip();
    s25fs_simple_cmd(FLASH_CMD_POWER_DOWN);
}

void s25fs_wake() {
    s25fs_simple_cmd(FLASH_CMD_POWER_UP);
    __delay_cycles(24);
}

/// Basic power-on self-test of serial flash, returning 1 on success.
uint8_t s25fs_post1() {
    volatile uint8_t status;
    volatile uint8_t initial_status = s25fs_get_status();

    s25fs_wr_en();
    status = s25fs_get_status();
    if (status != (initial_status | BIT1)) {
        return 0;
    }

    s25fs_wr_dis();
    status = s25fs_get_status();
    if (status != (initial_status & ~BIT1)) {
        return 0;
    }


    return s25fs_rdid() == FLASH_RDID_VAL_S25FS064S;
}

uint8_t s25fs_post2() {

    volatile uint8_t t;

    s25fs_wr_en();
    s25fs_erase_block_64kb(FLASH_ADDR_INTENTIONALLY_BLANK);
    do {
        if (s25fs_get_status() & 0b01100000) {
            // program or erase error
            s25fs_simple_cmd(FLASH_CMD_CLSR);
            return 0;
        }
    } while (s25fs_get_status() & FLASH_SR_WIP);

    s25fs_wr_en();
    t = 0xAB;
    s25fs_write_data(FLASH_ADDR_INTENTIONALLY_BLANK, &t, 1);
    do {
        if (s25fs_get_status() & 0b01100000) {
            // program or erase error
            s25fs_simple_cmd(FLASH_CMD_CLSR);
            return 0;
        }
    } while (s25fs_get_status() & FLASH_SR_WIP);

    return 1;
}



/// Decouple our pins from the flash chip, in order to allow external control.
void s25fs_hold_io() {
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

void s25fs_init_io() {
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

void s25fs_init() {
    // TODO: implement WP

    EUSCI_A_SPI_enable(EUSCI_A1_BASE);

    // Clear status register.
    s25fs_simple_cmd(FLASH_CMD_CLSR);

    uint8_t t = s25fs_read_any_register(0x000004);

    if (!(t & BIT3)) {
        // 4k erase needs to be DISABLED!
        s25fs_wr_en();
        s25fs_wr_register(0x000004, 0b00001000);
        t = s25fs_get_status();
        s25fs_block_while_wip();
        t = s25fs_get_status();
        __no_operation();
    }

    // Ok, now we're in a known state. Good to go.
}
