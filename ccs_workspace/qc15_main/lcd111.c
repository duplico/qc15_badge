/*
 * lcd111.c
 *
 * @duplico
 *
 */

/*
 * Screen stuff:
 *
 * 6.0 (GPIO)----------------------nRST
 * 6.1 (GPIO)----------------------SEL
 * 6.2 (GPIO)----------------------RW
 * 6.3 (GPIO)----------------------EN2
 * 6.4 (GPIO)----------------------EN1
 *                  _________
 * 4.5 (UCB1CLK) --|  Shift  |O0---DB0
 * 4.6 (UCB1TX) ---|Register |O1---DB1
 * 5.7 (GPIO) -----|74HC164D |O2---DB2
 *                 |         |O3---DB3
 *                 |         |O4---DB4
 *                 |         |O5---DB5
 *                 |         |O6---DB6
 *                 |_________|O7---DB7
 */
#include <stdint.h>

#include <msp430fr5972.h>
#include <driverlib.h>

#include "qc15.h"

#include "lcd111.h"
#include "util.h"

/// Initialize the shift register IO, and initialize (but not enable) the EUSCI.
void lcd111_sr_init_io() {
    // Shift register:
    /*
     * 4.5 (UCB1CLK) --|CLK
     * 4.6 (UCB1TX) ---|IN
     * 5.7 (GPIO) -----|nCLR
     */
    // CLK and TX are peripherals.
    P4SEL0 &= ~(BIT5|BIT6);
    P4SEL1 |= BIT5 | BIT6;

    // nCLR is GPIO
    P5SEL0 &= ~BIT7;
    P5SEL1 &= ~BIT7;
    P5DIR |= BIT7;

    P5OUT &= ~BIT7; // Pulse nCLR LOW
    P5OUT |= BIT7; // Bring nCLR HIGH (high = not cleared)

    // SPI EUSCI_B1 for the shift register
    EUSCI_B_SPI_initMasterParam p;
    p.clockPhase = EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
    p.clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
    p.msbFirst = EUSCI_B_SPI_MSB_FIRST;
    p.spiMode = EUSCI_B_SPI_3PIN;
    p.selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_ACLK;
    p.clockSourceFrequency = CS_getACLK();
    p.desiredSpiClock = 10000;
    EUSCI_B_SPI_initMaster(EUSCI_B1_BASE, &p);
}

// Initialize all the IO for the LCD driver.
void lcd111_init_io() {

    lcd111_sr_init_io();
    // DS1 and 2 share:
    // nRST 6.0
    // SEL  6.1
    // RW   6.2
    P6DIR |= BIT0|BIT1|BIT2;

    P6OUT &= ~BIT0; // Hold reset LOW when we turn on.
    P6OUT |= BIT1; // SEL is DONTCARE idle.
    P6OUT &= ~BIT2; // RW is always W, because we aren't even hooked up to read.

    // But have separate ENABLE lines:
    // EN1  6.4
    // EN2  6.3
    P6DIR |= BIT3|BIT4;
    P6OUT &= ~(BIT3+BIT4); // Enable is idle LOW.
}

/// Place byte `out` on the shift register.
void lcd111_sr_out(uint8_t out) {
    EUSCI_B_SPI_transmitData(EUSCI_B1_BASE, out);
    while (EUSCI_B_SPI_isBusy(EUSCI_B1_BASE));
}

void lcd111_wr(uint8_t lcd_id, uint8_t byte, uint8_t is_data) {
    // RS (6.1) controls command vs data:
    if (is_data)
        P6OUT |= BIT1;
    else
        P6OUT &= ~BIT1;
    // 40 nsec delay required, here. (at 5 MHz, each cycle is 20ns)
    // Bring EN high (must pulse for >230 ns)
    P6OUT |= (lcd_id ? BIT4 : BIT3);
    // Set valid data
    lcd111_sr_out(byte);
    // Bring EN low to read data
    P6OUT &= ~(lcd_id ? BIT4 : BIT3);
    // Hold data for >5ns


    // Keep the whole bus HIGH in between cycles.
    lcd111_sr_out(0xff);
}

/// Issue a command to one of the LCDs.
void lcd111_command(uint8_t lcd_id, uint8_t command) {
    lcd111_wr(lcd_id, command, 0);
    // We may need to do some additional waiting here. MOST commands take only
    //  10 cycles (of the LCD's onboard clock) to execute, in which case we
    //  DON'T need to do additional waiting. However, CL (clear display)
    //  takes 310 cycles, so we do need to wait a bit.
    if (command == LCD111_CMD_CLR) {
        // Clear.
        delay_millis(2); // Experimentally, this is about the perfect length.
    }

}

void lcd111_data(uint8_t lcd_id, uint8_t data) {
    lcd111_wr(lcd_id, data, 1);
    // All data operations take only 10 cycles (of the LCD's onboard clock)
    //  to run, so we don't need to add any additional waiting here.
}

void lcd111_sr_init() {
    // EUSCI_B_SPI_enable(EUSCI_B1_BASE);
    UCB1CTLW0 &= ~UCSWRST;
}

void lcd111_init() {
    lcd111_sr_init();
    lcd111_sr_out(0xff);
    // Pulse reset LOW, for at least 10 ms.
    P6OUT &= ~BIT0;
    delay_millis(10);
    P6OUT |= BIT0;
    // Reset is HIGH. Wait for a moment for things to stabilize.
    delay_millis(50);
    lcd111_command(0, 0x1c); // Power control: on
    lcd111_command(0, 0x14); // Display control: on
    lcd111_command(0, 0x28); // Display lines: 2, no doubling
    lcd111_command(0, 0x4f); // Contrast: dark
    lcd111_command(0, 0xe0); // Data address: 0

    lcd111_command(1, 0x1c); // Power control: on
    lcd111_command(1, 0x14); // Display control: on
    lcd111_command(1, 0x28); // Display lines: 2, no doubling
    lcd111_command(1, 0x4f); // Contrast: dark
    lcd111_command(1, 0xe0); // Data address: 0
}

/// Select cursor type. BIT2 inverting, BIT1 8th raster-row, BIT0 blink
void lcd111_cursor_type(uint8_t lcd_id, uint8_t cursor_type) {
    // Set 0b00001ABC
    //  A = inverting cursor
    //  B = 8th raster-row cursor
    //  C = blink cursor
    cursor_type = cursor_type & 0b0111;
    lcd111_command(lcd_id, 0b00001000 | cursor_type);
}

/// Set the cursor position to `pos`.
void lcd111_cursor_pos(uint8_t lcd_id, uint8_t pos) {
    if (pos > 23) pos = 23;
    lcd111_command(lcd_id, 0b11000000); // upper part to 0 (unused in these)
    lcd111_command(lcd_id, 0b11100000 | pos);
}

/// Clear the display and reset the address to 0.
void lcd111_clear(uint8_t lcd_id) {
    lcd111_command(lcd_id, 0x01); // Clear display and reset address.
}

/// Place `character` at the current cursor position in the LCD.
void lcd111_put_char(uint8_t lcd_id, char character) {
    lcd111_data(lcd_id, character);
}

/// Put a text buffer into the display, for `len` characters or until NULL.
void lcd111_put_text(uint8_t lcd_id, char *text, uint8_t len) {
    uint8_t i=0;
    while (text[i] && i<len) {
        lcd111_put_char(lcd_id, text[i++]);
    }
}

void lcd111_set_text(uint8_t lcd_id, char *text) {
    lcd111_clear(lcd_id);
    lcd111_put_text(lcd_id, text, 24);
}
