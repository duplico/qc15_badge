/*
 * ipc.c
 *
 *  Created on: Jun 17, 2018
 *      Author: george
 */

#include <stdint.h>

#include <msp430fr5972.h>
#include <driverlib.h>

#include "util.h"

void ipc_init_io() {
    // IPC:
    // 2.0 A0_TX
    // 2.1 A0_RX
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN0+GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    UCA0CTLW0 |= UCSWRST;
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
}

void ipc_init() {
    EUSCI_A_UART_enable(EUSCI_A0_BASE);
}
