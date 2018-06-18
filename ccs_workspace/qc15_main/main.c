#include <stdint.h>

#include <msp430fr5972.h>
#include <driverlib.h>

//#include "qc15.h"
#include "util.h"

#include "lcd111.h"
#include "flash.h"

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

    lcd111_init_io();

    // Screw post inputs
    // TODO: inputs, pull-ups
    GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN2 + GPIO_PIN3 + GPIO_PIN4); // TODO: eliminate driverlib calls


    // HT16D35B (LED Controller)
    // SDA  1.6 (UCB0)
    // SCL  1.7 (UCB0)
    // These require pull-up resistors.
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN6 + GPIO_PIN7);

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
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN0+GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    UCA0CTLW0 |= UCSWRST;
}

uint8_t led_values[24][3] = {
    {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff},
    {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff},
    {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff},
};

uint8_t led_mapping[24][3][2] = {{{2, 15}, {2, 16}, {2, 17}}, {{1, 15}, {1, 16}, {1, 17}}, {{0, 15}, {0, 16}, {0, 17}}, {{2, 23}, {2, 24}, {2, 25}}, {{1, 23}, {1, 24}, {1, 25}}, {{0, 23}, {0, 24}, {0, 25}}, {{2, 18}, {2, 19}, {2, 20}}, {{1, 18}, {1, 19}, {1, 20}}, {{0, 18}, {0, 19}, {0, 20}}, {{2, 11}, {2, 10}, {2, 9}}, {{1, 11}, {1, 10}, {1, 9}}, {{0, 11}, {0, 10}, {0, 9}}, {{2, 6}, {2, 7}, {2, 8}}, {{1, 6}, {1, 7}, {1, 8}}, {{0, 6}, {0, 7}, {0, 8}}, {{2, 12}, {2, 13}, {2, 14}}, {{0, 12}, {0, 13}, {0, 14}}, {{1, 12}, {1, 13}, {1, 14}}, {{0, 0}, {0, 2}, {0, 1}}, {{1, 0}, {1, 2}, {1, 1}}, {{2, 0}, {2, 2}, {2, 1}}, {{2, 3}, {2, 5}, {2, 4}}, {{1, 3}, {1, 5}, {1, 4}}, {{0, 3}, {0, 5}, {0, 4}}};
uint8_t led_col_mapping[3][28][2] = {{{18, 0}, {18, 2}, {18, 1}, {23, 0}, {23, 2}, {23, 1}, {14, 0}, {14, 1}, {14, 2}, {11, 2}, {11, 1}, {11, 0}, {16, 0}, {16, 1}, {16, 2}, {2, 0}, {2, 1}, {2, 2}, {8, 0}, {8, 1}, {8, 2}, {0, 0}, {0, 0}, {5, 0}, {5, 1}, {5, 2}, {0, 0}, {0, 0}}, {{19, 0}, {19, 2}, {19, 1}, {22, 0}, {22, 2}, {22, 1}, {13, 0}, {13, 1}, {13, 2}, {10, 2}, {10, 1}, {10, 0}, {17, 0}, {17, 1}, {17, 2}, {1, 0}, {1, 1}, {1, 2}, {7, 0}, {7, 1}, {7, 2}, {0, 0}, {0, 0}, {4, 0}, {4, 1}, {4, 2}, {0, 0}, {0, 0}}, {{20, 0}, {20, 2}, {20, 1}, {21, 0}, {21, 2}, {21, 1}, {12, 0}, {12, 1}, {12, 2}, {9, 2}, {9, 1}, {9, 0}, {15, 0}, {15, 1}, {15, 2}, {0, 0}, {0, 1}, {0, 2}, {6, 0}, {6, 1}, {6, 2}, {0, 0}, {0, 0}, {3, 0}, {3, 1}, {3, 2}, {0, 0}, {0, 0}}};

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

        __delay_cycles(500); // TODO - figure this out.

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
            __delay_cycles(100); // this is 50 kHz or so
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
    ht_send_two(HTCMD_BWGRAY_SEL, 0x00); // 0x01 = binary (LSB1=b/w; LSB0=gray)
    // Set column pin control for in-use cols (HTCMD_COM_PIN_CTL)
    ht_send_two(HTCMD_COM_PIN_CTL, 0b0000111); // comes through as 0b1111???
    // Set constant current ratio (HTCMD_I_RATIO)
    ht_send_two(HTCMD_I_RATIO, 0b0000); // This seems to be the max.
    // Set columns to 3 (0--2), and HIGH SCAN mode (HTCMD_COM_NUM)
    ht_send_two(HTCMD_COM_NUM, 0x02);

    // Set ROW pin control for in-use rows (HTCMD_ROW_PIN_CTL)
    // All rows are in use, except 27,26,22,21
    uint8_t row_ctl[] = {HTCMD_ROW_PIN_CTL, 0b00111001, 0xff, 0xff, 0xff};
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
//    ht_send_array(all_on_bw, 30);

    ht_send_two(HTCMD_SYS_OSC_CTL, 0b11); // Activate oscillator & display.

    //////////////////////////////////////////////////////////////////////

}

void led_send_bw() {
    uint8_t light_array[30] = {HTCMD_WRITE_DISPLAY, 0, 0};
    uint8_t col;
    uint8_t row;
    uint8_t bit;

    for (uint8_t led=0;led<24;led++) {
        for (uint8_t channel=0; channel<3; channel++) {
            col = led_mapping[led][channel][0];
            row = led_mapping[led][channel][1];
            // MSB is com0
            bit = 0b00000001 << (7-col);
            if (led_values[led][channel])
                light_array[row+2] |= bit; // set 0b1
            else
                light_array[row+2] &= ~bit; // set 0b0
        }
    }

    ht_send_array(light_array, 30);
}

void led_send_gray() {
    // the array, in this case, is:
    // COM0,ROW0 ... ROW27
    // COM1,ROW0 ...

    // So we only need to write the first three COMs.

    uint8_t light_array[30] = {HTCMD_WRITE_DISPLAY, 0x00, 0};

    for (uint8_t col=0; col<3; col++) {
        light_array[1] = 0x20*col;
        for (uint8_t row=0; row<28; row++) {
            uint8_t led_num = led_col_mapping[col][row][0];
            uint8_t rgb_num = led_col_mapping[col][row][1];

            light_array[row+2] = led_values[led_num][rgb_num];
        }

        ht_send_array(light_array, 30);
    }
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

void init_ipc() {
    // USCI A0 is our IPC UART:

    // 8-bit data
    // No parity
    // 1 stop
    // (8N1)
    // LSB first

    EUSCI_A_UART_initParam uart_param = {0};

    uart_param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK; // 1 MHz
    uart_param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION; // 1
    uart_param.clockPrescalar = 6;
    uart_param.firstModReg = 8;
    uart_param.secondModReg = 0x20; // 1/6/8/0x20 = 9600 @ 1 MHz
    uart_param.parity = EUSCI_A_UART_NO_PARITY;
    uart_param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    uart_param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    uart_param.uartMode = EUSCI_A_UART_MODE;

    EUSCI_A_UART_init(EUSCI_A0_BASE, &uart_param);
    EUSCI_A_UART_enable(EUSCI_A0_BASE);
}

//void test_flash() {
//    // Flash (2018):
//    // CS#      P3.6 (idle high)
//    // HOLD#    P3.3 (idle high)
//    // WP#       J.3 (idle high)
//    // CLK      3.6 (A1)
//    // SOMI     3.5 (A1)
//    // SIMO     3.4 (A1)
//
//    // CS# high normally
//    // HOLD# high normally
//    // WP# high normally (write protect when low)
//
//    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN6+GPIO_PIN3);
//    GPIO_setAsOutputPin(GPIO_PORT_PJ, GPIO_PIN3);
//    P3OUT |= BIT6+BIT3;
//    PJOUT |= BIT3;
//
//    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P3, GPIO_PIN4+GPIO_PIN5+GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION);
//
//    // Now, set up the SPI.
//    // Flash (2018):
//    // CLK      3.6 (A1)
//    // SOMI     3.5 (A1)
//    // SIMO     3.4 (A1)
//
//    EUSCI_A_SPI_initMasterParam ucaparam = {0};
//    ucaparam.clockPhase= EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
//    ucaparam.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
//    ucaparam.msbFirst = EUSCI_A_SPI_MSB_FIRST;
//    ucaparam.spiMode = EUSCI_A_SPI_3PIN;
//    ucaparam.selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK;
//    ucaparam.clockSourceFrequency = CS_getSMCLK();
//    ucaparam.desiredSpiClock = 100000;
//
//    EUSCI_A_SPI_disable(EUSCI_A1_BASE); // set WRST
//    EUSCI_A_SPI_initMaster(EUSCI_A1_BASE, &ucaparam); // init
//    EUSCI_A_SPI_enable(EUSCI_A1_BASE); // clear WRST
//
//    UCA1IFG &= ~UCRXIFG; // Clear any RXIFG.
//
//    P3OUT |= BIT6;
//    // Send 0x9f
//    while (!(UCA1IFG & UCTXIFG)); // wait for tx availabile
//    UCA1TXBUF = 0x9f;
//    while (!(UCA1IFG & UCRXIFG)); // wait for rx data available
//    UCA1IFG &= ~UCRXIFG; // ignore it.
//
//
//
//    // Read three bytes.
//
//}

void init() {
    init_clocks();
    init_io();
    lcd111_init();
}

void main (void)
{
    WDT_A_hold(WDT_A_BASE);
    init();

    lcd111_text(0, "Test 0");
    lcd111_text(1, "Test 1");

    while (1);

    init_clocks();
    volatile uint8_t s;
    init_io();
    lcd111_init();
    init_ht16d35b();
    init_flash();
    init_ipc();




    uint8_t led_on = 0;

//    GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN1); // 2.1 "RX"


    uint8_t all_on_bw[] = { HTCMD_WRITE_DISPLAY, 0, // 2 bytes
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, // (28 bytes of 1 - 28x8x1)
    };

    uint8_t all_off_bw[] = { HTCMD_WRITE_DISPLAY, 0, // 2 bytes
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, // (28 bytes of 1 - 28x8x1)
    };
//    ht_send_array(all_off_bw, 30);
    led_send_gray();

    while (1) {
    }
}
