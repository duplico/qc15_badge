#include <stdint.h>

#include <msp430fr5972.h>
#include "driverlib.h"

#include "flash.h"

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

    CS_setDCOFreq(0, 0); // Set DCO to 1 MHz

    CS_initClockSignal(CS_ACLK, CS_LFMODOSC_SELECT, CS_CLOCK_DIVIDER_1); // 39k
    CS_initClockSignal(CS_MCLK, CS_MODOSC_SELECT, CS_CLOCK_DIVIDER_1); // 5 M
//    CS_initClockSignal(CS_SMCLK, CS_MODOSC_SELECT, CS_CLOCK_DIVIDER_1); // 5 M
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1); // 1 M
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




    // HT16D35B (LED Controller)
    // SDA  1.6 (UCB0)
    // SCL  1.7 (UCB0)
    // These require pull-up resistors.
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P1,
        GPIO_PIN6 + GPIO_PIN7,
        GPIO_PRIMARY_MODULE_FUNCTION
    );

    EUSCI_B_I2C_initMasterParam param = {0};
    param.selectClockSource = EUSCI_B_I2C_CLOCKSOURCE_SMCLK;
    param.i2cClk = CS_getSMCLK();
    param.dataRate = EUSCI_B_I2C_SET_DATA_RATE_100KBPS;
    param.byteCounterThreshold = 1;
    param.autoSTOPGeneration = EUSCI_B_I2C_NO_AUTO_STOP;

    // Set slave addr
    // The slave address is: 0b110100X // X is floating right now.
    //  We may need a helper wire on A0.
    EUSCI_B_I2C_setSlaveAddress(EUSCI_B0_BASE, 0b1101000); //0b110100X

    EUSCI_B_I2C_initMaster(EUSCI_B0_BASE, &param);
    EUSCI_B_I2C_enable(EUSCI_B0_BASE);





    // Flash (2018):
    // CS#      P3.6 (idle high)
    // HOLD#    P3.3 (idle high)
    // WP#       J.3 (idle high)
    // CLK      3.6 (A1)
    // SOMI     3.5 (A1)
    // SIMO     3.4 (A1)

    // CS# high normally
    // HOLD# high normally
    // WP# high normally (write protect when low)

    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN3 + GPIO_PIN6);
    GPIO_setAsOutputPin(GPIO_PORT_PJ, GPIO_PIN3);
    P3OUT |= BIT6+BIT3; // Unheld, unselected
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
    ucaparam.desiredSpiClock = 100000;

    EUSCI_A_SPI_initMaster(EUSCI_A1_BASE, &ucaparam);
    EUSCI_A_SPI_enable(EUSCI_A1_BASE);

    // Buttons: 9.4, 9.5, 9.6, 9.7
    P9DIR &= ~(BIT4+BIT5+BIT6+BIT7); // inputs
    P9REN |= (BIT4+BIT5+BIT6+BIT7); // 0xf0
    P9OUT |= 0xf0; // pull up, please.

    // IPC:
    // 2.0 A0_TX
    // 2.1 A0_RX
    // 8-bit data
    // Even parity
    // MSB first
    //

    UCA0CTLW0 |= UCSWRST;

    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN0+GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    // USCI A0 is our IPC UART:
    EUSCI_A_UART_initParam uart_param = {0};

    uart_param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_ACLK;
    uart_param.clockPrescalar = 3;
    uart_param.firstModReg = 0;
    uart_param.secondModReg = 92;
    uart_param.parity = EUSCI_A_UART_NO_PARITY;
    uart_param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    uart_param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    uart_param.uartMode = EUSCI_A_UART_MODE;
    uart_param.overSampling = EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION;


    EUSCI_A_UART_init(EUSCI_A0_BASE, uart_param);
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

#define HTCMD_WRITE_DISPLAY 0x80
#define HTCMD_READ_DISPLAY  0x81
#define HTCMD_READ_STATUS   0x71
#define HTCMD_BWGRAY_SEL    0x31
#define HTCMD_COM_NUM       0x32
#define HTCMD_BLINKING      0x33
#define HTCMD_SYS_OSC_CTL   0x35
#define HTCMD_I_RATIO       0x36
#define HTCMD_GLOBAL_BRTNS  0x37
#define HTCMD_MODE_CTL      0x38
#define HTCMD_COM_PIN_CTL   0x41
#define HTCMD_ROW_PIN_CTL   0x42
#define HTCMD_DIR_PIN_CTL   0x43
#define HTCMD_SW_RESET      0xCC

void ht_send_array(uint8_t txdat[], uint8_t len) {
    // START
    UCB0CTLW0 |= UCTR; // Transmit mode.

    // Configure auto-stops:
    UCB0CTLW1 &= ~UCASTP_3; // Clear the auto-stop bits.
    UCB0CTLW1 |= UCASTP_2; // Auto-stop.

    UCB0CTLW0 |= UCSWRST; // Stop the I2C engine (clears STPIFG, too)
    UCB0TBCNT_L = len; // Set byte counter thresh (doesn't include addr)
    UCB0CTLW0 &= ~UCSWRST; // Re-start engine.

    UCB0IFG &= ~UCTXIFG; // Clear TX flag.
    UCB0CTLW0 |=  UCTXSTT; // Send a START.

    while (UCB0CTLW0 & UCTXSTT); // Wait for the address to finish sending.

    for (uint8_t i=0; i<len; i++) {
        // Wait for the TX buffer to become available again.
        while (!UCB0IFG & UCTXIFG) // While TX is unavailable, spin.
            __no_operation(); // TODO: We should watch for a NACK here. TODO: Demeter

        delay_millis(2); // TODO - figure this out.

        UCB0IFG &= ~UCTXIFG; // Clear TX flag.
        UCB0TXBUF = txdat[i]; // write dat.
    }

    while (!(UCB0IFG & UCSTPIFG)); // Wait for the auto-stop IFG to fire.

    // Disable auto-stop.
    UCB0CTLW1 &= ~UCASTP_3; // Clear the auto-stop bits.
    UCB0IFG &= ~UCSTPIFG; // Clear the auto-stop interrupt.
}

void ht_send_cmd_single(uint8_t cmd) {
    ht_send_array(&cmd, 1);
}

void ht_send_two(uint8_t cmd, uint8_t dat) {
    uint8_t v[2];
    v[0] = cmd;
    v[1] = dat;
    ht_send_array(v, 2);
}


// Read register status:
//  START, ADDR, HTCMD_READ_STATUS; reSTART, ADDR, READ_DUMMY, READ1, ..., READ20, NACK, STOP

void ht_read_reg(uint8_t reg[]) {
    // START
    UCB0CTLW0 |= UCTR; // Transmit.
    UCB0CTLW0 |=  UCTXSTT; // Send a START.

    while (!UCB0IFG & UCTXIFG) // Wait for the TX buffer to become available.
        __no_operation(); // TODO: We should watch for a NACK here???
    while (UCB0CTLW0 & UCTXSTT); // Wait for the address to finish sending.

    // Stage HTCMD_READ_STATUS to send.
    UCB0TXBUF = HTCMD_READ_STATUS;

    // Wait for TX to complete.
    while (!UCB0IFG & UCTXIFG) // While TX is unavailable, spin.
            __no_operation(); // TODO: We should watch for a NACK here. TODO: Demeter
    // TODO: We need to wait for it to actually COMPLETE, not just become available.
    //  For now, this delay is a stand-in for that.
    delay_millis(1);

    // Time to receive
    UCB0CTLW0 &= ~UCTR; // Set receive mode
    UCB0CTLW0 |=  UCTXSTT; // Send a RESTART. (but in RX mode)
    while (UCB0CTLW0 & UCTXSTT); // Wait for the address to finish sending.

    // Now, we're going to get a dummy byte, then 20 data bytes.
    for (uint8_t i=0; i<20; i++) {
        // Wait until the RXed item is ready:
        while (!(UCB0IFG & UCRXIFG0))
            __no_operation();
        // Now, we've received something. Let's grab it.
        reg[i] = UCB0RXBUF;
    }
    // Ok, now we have 20 bytes. There's probably something in our RX buffer
    //  already, too.
    UCB0CTLW0 |= UCTXSTP; // On our next receive cycle, send a NACK+STOP.
    reg[20] = UCB0RXBUF;

    while (UCB0CTLW0 & UCTXSTP); // Wait for the STOP to finish sending.

    // The other end is a circular buffer, so just wait for the stop to take.
    while(UCB0IFG & UCRXIFG0) {
        volatile uint8_t i;
        i = UCB0RXBUF;
        delay_millis(1);
    }
}

void init_ht16d35b() {
    // On POR:
    //  All registers reset to default, but DDRAM not cleared
    //  Oscillator off
    //  COM and ROW high impedance
    //  LED display OFF.

    // In GRAY MODE (which we're using), the display RAM is 28x8x6.
    // There's some extra RAM gizmos we probably don't really care about:
    //  Fade
    //  UCOM
    //  USEG
    //  Matrix masking

    //////////////////////////////////////////////////////////////////////

    // Check for a power-on fault...
    if (UCB0STATW & UCBBUSY) {
        // Bus is busy because the slave is being silly.
        //  I need to grab control of the clock line and send some clicks
        //  so it flushes out its logic brain.

        UCB0CTLW0 |= UCSWRST; // Stop the I2C engine
        GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN7);
        for (uint8_t i=0; i<11; i++) {
            // tick tock tick tock
            P1OUT |= BIT7;
            __delay_cycles(1000); // this is 5 kHz or so
            P1OUT &= ~BIT7;
        }
        GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
        UCB0CTLW0 &= ~UCSWRST; // Re-start engine.
    }

    // SW Reset (HTCMD_SW_RESET)
    ht_send_cmd_single(HTCMD_SW_RESET); // workie.

    volatile uint8_t ht_status_reg[22] = {0};
    ht_read_reg((uint8_t *) ht_status_reg);
    __no_operation();

    // Set global brightness (HTCMD_GLOBAL_BRTNS)
    ht_send_two(HTCMD_GLOBAL_BRTNS, 0x0f); // 0x40 is the most
    // Set BW/Binary display mode. TODO: Make it gray
    ht_send_two(HTCMD_BWGRAY_SEL, 0x01);
    // Set column pin control for in-use cols (HTCMD_COM_PIN_CTL)
    ht_send_two(HTCMD_COM_PIN_CTL, 0b0000111); // comes through as 0b1111???
    // Set constant current ratio (HTCMD_I_RATIO)
    ht_send_two(HTCMD_I_RATIO, 0b0000); // This seems to be the max.
    // Set columns to 3 (0--2), and HIGH SCAN mode (HTCMD_COM_NUM)
    ht_send_two(HTCMD_COM_NUM, 0x02);

    // Set ROW pin control for in-use rows (HTCMD_ROW_PIN_CTL)
    uint8_t row_ctl[] = {HTCMD_ROW_PIN_CTL, 0xff, 0xff, 0xff, 0xff};
    // TODO: rows 21,22,26,27 are unused
    ht_send_array(row_ctl, 5);
    ht_send_two(HTCMD_SYS_OSC_CTL, 0b10); // Activate oscillator.

    // Check conf: TODO: POST
    ht_read_reg((uint8_t *) ht_status_reg);
    __no_operation();


    uint8_t all_on_bw[] = { HTCMD_WRITE_DISPLAY, 0, // 2 bytes
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, // (28 bytes of 1 - 28x8x1)
    };
    ht_send_array(all_on_bw, 30);

    ht_send_two(HTCMD_SYS_OSC_CTL, 0b11); // Activate oscillator & display.

    //////////////////////////////////////////////////////////////////////

}

void light_channel(uint8_t ch) {
    uint8_t light_array[30] = {HTCMD_WRITE_DISPLAY, 0, 0};

    uint8_t byt=0;
    uint8_t bit=0;

    byt= 30 - ch/8;
    bit= ch % 8;

    light_array[byt] = (0x01 << bit);

    ht_send_array(light_array, 30);
}

void main (void)
{
    WDT_A_hold(WDT_A_BASE);
    init_clocks();
    volatile uint8_t s;
    init_io();
    init_lcd();
    init_ht16d35b();
    init_flash();

    lcd_text(0, "TEST test");
    lcd_text(1, "SCREEN2 test");

    uint8_t led_on = 0;

    while (1) {
        delay_millis(25);
        light_channel(led_on);
        led_on = (led_on + 1) % 224;

        P1OUT ^= (BIT6 + BIT7);
        P7OUT ^= (BIT2 + BIT3 + BIT4);
        if ((P9IN & 0xf0) != 0xf0) {
            __no_operation();
        }
    }
}
