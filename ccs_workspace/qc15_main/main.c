#include <stdint.h>

#include <msp430fr5972.h>
#include <driverlib.h>

// TODO: Create:
//#include "qc15.h"
// TODO: Create a conf.h
#include "util.h"

#include "lcd111.h"
#include "ht16d35b.h"
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
     *  - LFXTCLK (not available)
     *  - VLOCLK (very low power (100 nA), 10-kHz)
     *  - DCOCLK (selectable DCO)
     *  - MODCLK (low-power (25 uA), 5 MHz)
     *  - LFMODCLK (MODCLK/128, 39 kHz)
     *  - HFXTCLK (not available)
     */

    CS_setDCOFreq(1, 4); // Set DCO to 16 MHz

    CS_initClockSignal(CS_ACLK, CS_LFMODOSC_SELECT, CS_CLOCK_DIVIDER_1); // 39k
    // NB: If MCLK is over 8 MHz we have to mess around with FRAM wait states.
    //     I'd rather not do that.
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_2); // 8 M
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_8); // 1 M
}

void init_io() {
    // The magic make-it-work command.
    //  Otherwise everything is stuck in high-impedance forever.
    PMM_unlockLPM5();

    lcd111_init_io();
    ht16d_init_io();

    // Screw post inputs
    // TODO: inputs, pull-ups
    GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN2 + GPIO_PIN3 + GPIO_PIN4); // TODO: eliminate driverlib calls

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

void init() {
    init_clocks();
    init_io();
    lcd111_init();
    ht16d_init();
}

void main (void)
{
    WDT_A_hold(WDT_A_BASE);
    init();

    lcd111_text(0, "Test 0");
    lcd111_text(1, "Test 1");
    led_send_gray();

    while (1);
    init_flash();
    init_ipc();
}
