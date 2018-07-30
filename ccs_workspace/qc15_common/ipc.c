/*
 * ipc.c
 *
 *  Created on: Jul 7, 2018
 *      Author: george
 */

#include <stdint.h>
#include <string.h>

#include <msp430.h>
#include <driverlib.h>

#include "qc15.h"

#include "ipc.h"
#include "util.h"

volatile uint8_t f_ipc_rx = 0;

volatile uint8_t ipc_state = 0;
uint8_t ipc_tx_index = 0;
uint8_t ipc_tx_len = 0;
uint8_t ipc_tx_buf[IPC_MSG_LEN_MAX+2] = {0};
volatile uint8_t ipc_rx_index = 0;
volatile uint8_t ipc_rx_len = 0;
volatile uint8_t ipc_rx_buf[IPC_MSG_LEN_MAX+2] = {0};

uint8_t ipc_tx_byte(uint8_t tx_byte) {
    return ipc_tx(&tx_byte, 1);
}

uint8_t ipc_tx_op_buf(uint8_t op, uint8_t *tx_buf, uint8_t len) {
    if (ipc_state & IPC_STATE_TX_MASK) {
        // TX already in progress. Abort.
        return 0;
    }

    if (len > IPC_MSG_LEN_MAX) {
        len = IPC_MSG_LEN_MAX;
    }

    ipc_tx_buf[0] = op;
    memcpy(&ipc_tx_buf[1], tx_buf, len);

    crc16_append_buffer(ipc_tx_buf, len+1);

    ipc_tx_index = 0;
    ipc_tx_len = len+3;

    // Begin TX by sending the SYNC word.
    UCA0TXBUF = IPC_SYNC_WORD;
    // Next we will need to send the length.
    ipc_state |= IPC_STATE_TX_LEN;
    return 1;
}

uint8_t ipc_tx(uint8_t *tx_buf, uint8_t len) {
    if (ipc_state & IPC_STATE_TX_MASK) {
        // TX already in progress. Abort.
        return 0;
    }

    if (len > IPC_MSG_LEN_MAX) {
        len = IPC_MSG_LEN_MAX;
        while (1); // TODO: ASSERT/SPIN
    }

    memcpy(ipc_tx_buf, tx_buf, len);

    crc16_append_buffer(ipc_tx_buf, len);

    ipc_tx_index = 0;
    ipc_tx_len = len+2;

    // Begin TX by sending the SYNC word.
    UCA0TXBUF = IPC_SYNC_WORD;
    // Next we will need to send the length.
    ipc_state |= IPC_STATE_TX_LEN;
    return 1;
}

/// Validate and copy IPC message into `rx_buf`, returning 1 if successful.
uint8_t ipc_get_rx(uint8_t *rx_buf) {
    // Once we either invalidate ipc_rx_buf, or copy it into rx_buf, we can
    //  allow it to be overwritten. But not until then. And we have to be
    //  cautious about this, since it's guarding against an interrupt.

    // If the CRC fails,
    if (!crc16_check_buffer((uint8_t *)ipc_rx_buf, ipc_rx_len-2)) {
        ipc_state &= ~IPC_STATE_RX_MASK;
        return 0;
    }

    memcpy(rx_buf, (uint8_t *)ipc_rx_buf, ipc_rx_len-2);
    ipc_state &= ~IPC_STATE_RX_MASK;
    return 1;
}

void ipc_init() {
    // USCI A0 is our IPC UART:
    //  (on both chips! Yay!)

    // 8-bit data
    // No parity
    // 1 stop
    // (8N1)
    // LSB first
    // Baud Rate calculation:
    // 1000000/(9600) = 104.16667
    // OS16 = 1
    // UCBRx = INT(N/16) = 6.5104,
    // UCBRFx = INT([N/16 - INT(N/16)] x 16) = 8.1667
    // UCBRSx = 0x11 (per Table 22-4 but T22-5 says 0x20 will also work)

    UCA0CTLW0 |= UCSWRST;
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BR0 = 6; // 1000000/9600/16
    UCA0BR1 = 0x00;
    UCA0MCTLW = 0x2000 | UCOS16 | UCBRF_8;
    UCA0CTLW0 &= ~UCSWRST;
    UCA0IE |= UCTXIE | UCRXIE;
}

#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    static volatile uint8_t rx_byte = 0;
    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
    {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
        rx_byte = UCA0RXBUF;
        // RX IRQ

        // The following if statement should be read in reverse order of
        //  conditions. That is, proper execution of the IPC protocol should
        //  hit each if condition starting from the `else` and going up.
        if (ipc_state & IPC_STATE_RX_HOLD) {
            // We have to ignore these, because the contents of
            //  our RX buffer are not allowed to go stale yet.
            break;
        } else if (ipc_state & IPC_STATE_RX_BUSY) {
            // We are already receiving something.
            ipc_rx_buf[ipc_rx_index] = rx_byte;
            ipc_rx_index++;
            if (ipc_rx_index == ipc_rx_len) {
                // RX completed.
                ipc_state &= ~IPC_STATE_RX_MASK;
                ipc_state |= IPC_STATE_RX_HOLD;
                f_ipc_rx = 1;
                LPM_EXIT;
                break;
            }
        } else if (ipc_state & IPC_STATE_RX_LEN) {
            // We are ready to accept the length of the message.
            if (rx_byte < IPC_MSG_LEN_MAX) {
                // Valid length
                ipc_state &= ~IPC_STATE_RX_LEN;
                ipc_state |= IPC_STATE_RX_BUSY;
                ipc_rx_len = rx_byte;
            } else {
                // Invalid length. Cancel RX.
                ipc_state &= ~IPC_STATE_RX_MASK;
            }
        } else {
            // This should be the first byte we receive.
            if (rx_byte == IPC_SYNC_WORD) {
                // Valid first byte.
                ipc_state |= IPC_STATE_RX_LEN;
                ipc_rx_index = 0;
                ipc_rx_len = 0;
            } else {
                // Invalid first byte.
                // Cancel RX
                ipc_state &= ~IPC_STATE_RX_MASK;
            }
        }
        break;
    case USCI_UART_UCTXIFG:
        // TX IRQ
        // We just finished sending something.
        if (ipc_state & IPC_STATE_TX_LEN) {
            // We just finished sending the SYNC word, and now we
            //  need to send the length of our message.
            UCA0TXBUF = ipc_tx_len;
            ipc_state &= ~IPC_STATE_TX_MASK;
            ipc_state |= IPC_STATE_TX_READY;
        } else if (ipc_state & IPC_STATE_TX_READY) {
            // Time to send the first data byte.
            UCA0TXBUF = ipc_tx_buf[ipc_tx_index];
            ipc_state &= ~IPC_STATE_TX_MASK;
            ipc_state |= IPC_STATE_TX_BUSY;
        } else if (ipc_state & IPC_STATE_TX_BUSY) {
            // We just sent ipc_tx_index of ipc_tx_buf.
            ipc_tx_index++;
            if (ipc_tx_index == ipc_tx_len) {
                // We just finished sending our message.
                //  Go idle.
                ipc_state &= ~IPC_STATE_TX_MASK;
            } else {
                // It's time to send the next byte.
                UCA0TXBUF = ipc_tx_buf[ipc_tx_index];
            }
        } else {
            // No operation. If we're not in a correct TX state for this,
            //  it's quite likely this interrupt was generated by
            //  the initialization of the UART.
        }
        break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
    default: break;
    }
}
