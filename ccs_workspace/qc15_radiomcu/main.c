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

// TODO: Determine startup clock config.
void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(5000);
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
//     * P1.4 IPC RX
//     * P1.5 IPC TX
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P1,
        GPIO_PIN4 + GPIO_PIN5,
        GPIO_PRIMARY_MODULE_FUNCTION
        );

    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_ACLK;
    param.clockPrescalar = 3;
    param.firstModReg = 0;
    param.secondModReg = 92;
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION;

    if (STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param)) {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);

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

    init_io(); // TODO: harmonize the idiom for the init fns

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
    rfm75_post();

    while (1) {
        if (P2IN & BIT2) {
            // switch is HIGH:
            __no_operation();
        }
        delay_millis(500);
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
