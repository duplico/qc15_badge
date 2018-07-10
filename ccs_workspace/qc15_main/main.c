#include <stdint.h>

#include <msp430fr5972.h>
#include <driverlib.h>
#include <s25flash.h>

//#include "qc15.h"

#include "util.h"

void main (void)
{
    WDT_A_hold(WDT_A_BASE);
    PMM_unlockLPM5();

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

    // IPC TX/RX GPIO
//    P2SEL0 &= ~(BIT0+BIT1);
//    P2SEL1 &= ~(BIT0+BIT1);
    // IPC TX/RX Peripheral
    P2SEL0 |=  (BIT0+BIT1);
    P2SEL1 &= ~(BIT0+BIT1);

    P2DIR |= BIT0; // UCA0TXD
    P2DIR &= ~BIT1; // UCA0RXD

    volatile uint8_t rx_in = 0;
    volatile uint32_t sclk = CS_getSMCLK();
    __no_operation();
    volatile uint8_t rx_in_last = 0;

    while (1) {
        while (!(UCA0IFG & UCRXIFG)); // wait to RX
        rx_in = UCA0RXBUF;
        __no_operation();
    }
}
