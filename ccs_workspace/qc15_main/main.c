#include <msp430fr5972.h>
#include "driverlib.h"
#include <stdint.h>

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

void init_clocks() {
    /*
     * On power-on, the following defaults apply in the clock system:
     *  - LFXT selected as oscillator source for LFXTCLK (not populated)
     *  - ACLK: undivided, LFXTCLK
     *  - MCLK: DCOCLK (/8)
     *  - SMCLK:DCOCLK (/8)
     *  - LFXIN/LFXOUT are GPIO and LFXT is disabled
     */

    /*
     * Available sources:
     *  - LFXTCLK (not used)
     *  - VLOCLK (very low power, 10-kHz)
     *  - DCOCLK (selectable DCO)
     *  - MODCLK (low-power, 5 MHz)
     *  - HFXTCLK (not available)
     */

    CS_initClockSignal(CS_ACLK, CS_LFMODOSC_SELECT, CS_CLOCK_DIVIDER_1); // 39k
    CS_initClockSignal(CS_MCLK, CS_MODOSC_SELECT, CS_CLOCK_DIVIDER_1); // 5 M
    CS_initClockSignal(CS_SMCLK, CS_MODOSC_SELECT, CS_CLOCK_DIVIDER_1); // 5 M
}

void init_io() {
    // The magic make-it-work command.
    //  Otherwise everything is stuck in high-impedance forever.
    PMM_unlockLPM5();

    // Shift register:
    /*
     * 4.5 (UCB1CLK) --|CLK
     * 4.6 (UCB1TX) ---|IN
     * 5.7 (GPIO) -----|nCLR
     */
    GPIO_setAsPeripheralModuleFunctionOutputPin(
            GPIO_PORT_P4,
            GPIO_PIN5 | GPIO_PIN6,
            GPIO_SECONDARY_MODULE_FUNCTION
    );
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN7);
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
    EUSCI_B_SPI_enable(EUSCI_B1_BASE);

    // 1.6, 1.7: SDIO and SCLK for LED controller:
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN6 + GPIO_PIN7);
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN6);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN7);

    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN3);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN3);

    GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN2 + GPIO_PIN3 + GPIO_PIN4);

    // DS1 and 2 share:
    // nRST 6.0
    // SEL  6.1
    // RW   6.2
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2);
    P6OUT &= ~BIT0; // Hold reset LOW when we turn on.
    P6OUT |= BIT1+BIT2; // SEL and RW are DONTCARE idle.

    // But have separate ENABLE lines:
    // EN1  6.4
    // EN2  6.3
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN4);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN3);
    P6OUT &= ~(BIT3+BIT4); // Enable is idle LOW.
}

void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(5000);
        mils--;
    }
}

void set_sr_out(uint8_t out) {
    EUSCI_B_SPI_transmitData(EUSCI_B1_BASE, out);
    while (EUSCI_B_SPI_isBusy(EUSCI_B1_BASE));
        __no_operation(); // Spin while it sends.
}

void lcd_command(uint8_t lcd_id, uint8_t command) {
    // Bring RS LOW for command and RW LOW for WRITE
    P6OUT &= ~BIT1;
    P6OUT &= ~BIT2;
    // 40 nsec delay required, here. (at 5 MHz, each cycle is 20ns)
    __delay_cycles(2); // TODO
    // Bring EN high (must pulse for >230 ns)
    P6OUT |= (lcd_id ? BIT4 : BIT3);
    // Set valid data
    set_sr_out(command);
    // Bring EN low to read data
    P6OUT &= ~(lcd_id ? BIT4 : BIT3);
    // Hold data for >5ns
    __delay_cycles(1);

    // Hold on for a moment while it works
    //  We can't read, in this configuration, so we can't
    //  poll the busy signal.
    delay_millis(50);

    // Keep the whole bus HIGH in between cycles.
    set_sr_out(0xff);
}

void lcd_data(uint8_t lcd_id, uint8_t data) {
    // Bring RS HIGH for data and RW LOW for WRITE
    P6OUT |= BIT1;
    P6OUT &= ~BIT2;
    // 40 nsec delay required, here. (at 5 MHz, each cycle is 20ns)
    __delay_cycles(2); // TODO
    // Bring EN high (must pulse for >230 ns)
    P6OUT |= (lcd_id ? BIT4 : BIT3);
    // Set valid data
    set_sr_out(data);
    // Bring EN low to read data
    P6OUT &= ~(lcd_id ? BIT4 : BIT3);
    // Hold data for >5ns
    __delay_cycles(1);

    // Hold on for a moment while it works
    //  We can't read, in this configuration, so we can't
    //  poll the busy signal.
    delay_millis(1);

    // Keep the whole bus HIGH in between cycles.
    set_sr_out(0xff);
}

void init_lcd() {
    set_sr_out(0xff);
    // Pulse reset LOW, for at least 10 ms.
    P6OUT &= ~BIT0;
    delay_millis(10);
    P6OUT |= BIT0;
    // Reset is HIGH. Wait for a moment for things to stabilize.
    delay_millis(50);
    lcd_command(0, 0x1c); // Power control: on
    lcd_command(0, 0x14); // Display control: on
    lcd_command(0, 0x28); // Display lines: 2, no doubling
    lcd_command(0, 0x4f); // Contrast: dark
    lcd_command(0, 0xe0); // Data address: 0

    lcd_command(1, 0x1c); // Power control: on
    lcd_command(1, 0x14); // Display control: on
    lcd_command(1, 0x28); // Display lines: 2, no doubling
    lcd_command(1, 0x4f); // Contrast: dark
    lcd_command(1, 0xe0); // Data address: 0
}

void lcd_text(uint8_t lcd_id, char *text) {
    uint8_t i=0;
    while (text[i])
        lcd_data(lcd_id, text[i++]);
}

void main (void)
{
    WDT_A_hold(WDT_A_BASE);
    init_clocks();
    init_io();
    init_lcd();

    lcd_text(0, "TEST test");
    lcd_text(1, "SCREEN2 test");

    while (1) {
        delay_millis(1500);
        P1OUT ^= (BIT6 + BIT7);
        P7OUT ^= (BIT2 + BIT3 + BIT4);
    }
}
