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

//     * P1.0 radio CS (GPIO)
//     * P1.1 radio CLK (peripheral)
//     * P1.2 radio SIMO (peripheral)
//     * P1.3 radio SOMI (peripheral)
//     * P1.4 IPC TX
//     * P1.5 IPC RX
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P1,
        GPIO_PIN4 + GPIO_PIN5,
        GPIO_PRIMARY_MODULE_FUNCTION
        );

//     * P1.6 radio EN
//     * P1.7 radio IRQ
    P1SEL0 = 0b00111110; // MSB
    P1SEL1 = 0b00000000; // LSB
    P1DIR |= BIT6+BIT0;
    P1DIR &= ~BIT7;
//     * P2.0 XTAL
//     * P2.1 XTAL
//     * P2.2 switch (needs pull-UP, active low) (right is high)
    P2SEL1 = 0b011; // MSB
    P2SEL0 = 0b000; // LSB
    P2DIR &= ~BIT2; // Switch pin set to input.
    P2REN |= BIT2; // Switch resistor enable
    P2OUT |= BIT2; // Switch resistor pull UP direction
}

void main (void)
{
    //Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);

    init_io();

    // On boot, the clock system is as follows:
    // * MCLK and SMCLK -> DCOCLKDIV (divided DCO) (1 MHz)
    // * ACLK           -> REFO (32k internal oscillator)

    //Initializes the XT1 crystal oscillator with no timeout
    //In case of failure, code hangs here.
    //For time-out instead of code hang use CS_turnOnXT1LFWithTimeout()
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

	//Enable oscillator fault interrupt
    SFR_enableInterrupt(SFR_OSCILLATOR_FAULT_INTERRUPT);

    rfm75_init();
//    rfm75_post();

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

#pragma vector=UNMI_VECTOR
__interrupt
void NMI_ISR(void)
{
  do {
    // If it still can't clear the oscillator fault flags after the timeout,
    // trap and wait here.
    status = CS_clearAllOscFlagsWithTimeout(1000);
  } while(status != 0);
}
