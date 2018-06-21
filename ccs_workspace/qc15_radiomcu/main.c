/*
 * main.c
 *
 *  Created on: Jun 15, 2018
 *      Author: @duplico
 */


/*
 * P1.0 radio CS
 * P1.1 radio CLK
 * P1.2 radio SIMO
 * P1.3 radio SOMI
 * P1.4 IPC TX (A0TX)
 * P1.5 IPC RX (A0RX)
 * P1.6 radio EN
 * P1.7 radio IRQ
 * P2.0 XTAL
 * P2.1 XTAL
 * P2.2 switch (right is high)
 *   |----/\/\/\/\/-----VCC
 */

#include <stdint.h>

#include "driverlib.h"
#include <msp430fr2422.h>

#include "rfm75.h"

uint16_t status;

void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(1000);
        mils--;
    }
}

void init_io() {
    // The magic FRAM make-it-work command:
    PMM_unlockLPM5();

    // Radio IO is all set inside the radio driver. For now.

    // Port 1:
    // -------
    // * P1.0 RFM CSN (GPIO)    (configured in radio driver)
    // * P1.1 RFM CLK (Primary) (configured in radio driver)
    // * P1.2 RFM MO  (Primary) (configured in radio driver)
    // * P1.3 RFM MI  (Primary) (configured in radio driver)
    // * P1.4 IPC TX  (Primary)
    // * P1.5 IPC RX  (Primary)
    // * P1.6 RFM CE  (GPIO)    (configured in radio driver)
    // * P1.7 RFM IRQ (GPIO in) (configured in radio driver)

    // IPC TX/RX
    P1SEL0 |= BIT4+BIT5;
    P1SEL0 &= ~(BIT4+BIT5); // unneeded but whatever.

    // Port 2:
    // -------
    // * P2.0 LFXT (Primary)
    // * P2.1 LFXT (Primary)
    // * P2.2 PWSW (GPIO in w/ pull-up) (right/down is HIGH)
    //
    //   These are all the usable GPIO pins on the device.

    P2SEL1 = 0b011; // MSB
    P2SEL0 = 0b000; // LSB
    P2DIR &= ~BIT2; // Switch pin set to input.
    P2REN |= BIT2;  // Switch resistor enable
    P2OUT |= BIT2;  // Switch resistor pull UP direction
}

void init_clocks() {
    // CLOCK SOURCES
    // =============

    // Fixed sources:
    //      REFO     32k Integrated 32 kHz RC oscillator
    //      VLO      10k Very low power low-frequency oscillator
    //      MODOSC   5M  for MODCLK

    // Configurable sources:
    // DCO  (Digitally-controlled oscillator)
    //  This defaults to 16 MHz. We'll keep this.

    // LFXT (Low frequency external crystal)
    CS_turnOnXT1LF(
            CS_XT1_DRIVE_0
    );

    CS_initClockSignal(
            CS_ACLK,
            CS_XT1CLK_SELECT,
            CS_CLOCK_DIVIDER_1
    );

    //clear all OSC fault flag
    CS_clearAllOscFlagsWithTimeout(1000);
    // TODO: OSC flags in POST routine, please.

    // SYSTEM CLOCKS
    // =============

    // MCLK (1 MHz)
    //  All sources but MODOSC are available at up to /128
    //  Initializes to DCOCLKDIV (divided DCO) (1 MHz)
    //  This is fine.

    // SMCLK
    //  Derived from MCLK with divider up to /8
    //  Initializes to MCLK/1, which we'll keep.

    // MODCLK (5 MHz)
    //  This comes from MODOSC

    // ACLK
    //  Initializes to REFO, which is ~ 32k.
    //  This is OK, but we'd rather have it connected to our watch crystal,
    //   which will give us a more precise 32k signal.
}

void main (void)
{
    //Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);
    init_io();
    init_clocks();

    rfm75_init();

    // Set the global interrupt enable:
    __bis_SR_register(GIE);

    while (1) {
        rfm75_tx();
        delay_millis(1000);
    }

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

//    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN4); // "TX"
    uint8_t msg = 0;

    while (1) {
//        if (UCA0IFG & UCRXIFG) {
//            msg = UCA0RXBUF;
//            while (!(UCA0IFG & UCTXIFG)); // wait for TX buffer availability
//            UCA0TXBUF = msg;
//        }
//        delay_millis(1);
    }

    //Enter LPM3 w/ interrupts
    __bis_SR_register(LPM3_bits + GIE);

    //For debugger
    __no_operation();
}
