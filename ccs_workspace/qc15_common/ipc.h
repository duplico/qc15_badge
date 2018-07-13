/*
 * ipc.h
 *
 *  Created on: Jul 7, 2018
 *      Author: george
 */

#ifndef IPC_H_
#define IPC_H_

#define IPC_MSG_LEN_MAX sizeof(qc15status)+4
#define IPC_SYNC_WORD 0xea

/*
 * IPC protocol:
 *  Send IPC_SYNC_WORD
 *  Send length
 *  Send payload
 *  Send CRC16
 */

// From MAIN to RADIO:
#define IPC_MSG_STATS_ANS 0x01
#define IPC_MSG_REBOOT 0x02

// From RADIO to MAIN:
#define IPC_MSG_STATS_REQ 0x90
#define IPC_MSG_POST 0xa0
#define IPC_MSG_SWITCH 0xb0

// IPC tasks:
//  * Startup      (R->M) (M->R)
//  *  R->M:
//  *  M->R: Stats update
//  * Time setting (M->R)
//  * Time event (R->M)
//  * Update stats (M->R)
//  * (Successful download)
//  * (Successful upload)
//  * Gaydar updates:
//  *  Person arrived (w/ name)
//  *  Person departs (id only)
//  * Power switch status update
//  * Profit

#define IPC_STATE_IDLE    0b0000
#define IPC_STATE_RX_LEN  0b0010
#define IPC_STATE_RX_BUSY 0b0100
#define IPC_STATE_RX_HOLD 0b1000
#define IPC_STATE_RX_MASK (IPC_STATE_RX_LEN | IPC_STATE_RX_BUSY | IPC_STATE_RX_HOLD)
#define IPC_STATE_TX_LEN   0b00010000
#define IPC_STATE_TX_READY 0b00100000
#define IPC_STATE_TX_BUSY  0b01000000
#define IPC_STATE_TX_MASK (IPC_STATE_TX_LEN | IPC_STATE_TX_READY | IPC_STATE_TX_BUSY)

//extern uint8_t ipc_state;
extern volatile uint8_t f_ipc_rx;

void ipc_init();
uint8_t ipc_tx(uint8_t *tx_buf, uint8_t len);
uint8_t ipc_tx_byte(uint8_t tx_byte);
uint8_t ipc_tx_op_buf(uint8_t op, uint8_t *tx_buf, uint8_t len);
uint8_t ipc_get_rx(uint8_t *rx_buf);

#endif /* IPC_H_ */
