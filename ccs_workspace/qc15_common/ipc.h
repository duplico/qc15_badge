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

#define IPC_STATE_IDLE    0b0000
#define IPC_STATE_RX_LEN  0b0010
#define IPC_STATE_RX_BUSY 0b0100
#define IPC_STATE_RX_HOLD 0b1000
#define IPC_STATE_RX_MASK (IPC_STATE_RX_LEN | IPC_STATE_RX_BUSY | IPC_STATE_RX_HOLD)
#define IPC_STATE_TX_LEN   0b00010000
#define IPC_STATE_TX_READY 0b00100000
#define IPC_STATE_TX_BUSY  0b01000000
#define IPC_STATE_TX_MASK (IPC_STATE_TX_LEN | IPC_STATE_TX_READY | IPC_STATE_TX_BUSY)

// IPC tasks:
//  [x] Reboot (M->R)
//  [x] POST/bootstrap (R->M)
//  [ ] Time setting (manual, not time virus) (M->R)
//  [ ] Time event (R->M)
//  [x] Update status (M->R)
//  [ ] Make me connectable (M->R)      (GD_EN)
//  [ ] Attempt to download (M->R)      (GD_DL)
//  [ ] Successful download (R->M)      (GD_DL)
//  [ ] Successful upload (R->M)        (GD_UL)
//  [x] Person arrived (w/ name) (R->M) (GD_ARR)
//  [x] Person departs (id only) (R->M) (GD_DEP)
//  [ ] Get next neighbor id (M->R)     (ID_NEXT)
//  [ ] Next neighbor id (R->M)         (ID_NEXT)
//  [x] Power switch status update (R->M)
//  [ ] ????
//  [ ] Profit

// Single-byte messages:
#define IPC_MSG_STATS_REQ 0x90
#define IPC_MSG_POST 0xa0
#define IPC_MSG_SWITCH 0xb0
/// Request the radio MCU to set ourselves as connectable.
#define IPC_MSG_GD_EN 0xc0
/// A request for the recipient to reboot.
#define IPC_MSG_REBOOT 0x60

// Buffer messages:
/// A badge has arrived in range.
#define IPC_MSG_GD_ARR 0x10 // cmd, id, name
/// A badge has departed.
#define IPC_MSG_GD_DEP 0x20 // cmd, id
/// Occurs when we've downloaded another badge, or to request a download.
/**
 ** A true-evaluating lower nibble indicates success, whereas a false one
 ** indicates failure. When requesting a download, the lower nibble is
 ** dontcare.
 */
#define IPC_MSG_GD_DL 0x30
/// Occurs when another badge has downloaded from us
#define IPC_MSG_GD_UL 0x40
/// Request or return the next neighbor ID before or after the current one.
/**
 ** A true-evaluating lower nibble indicates that the "next" ID is being
 ** requested, whereas a false-evaluating one indicates "previous". For the
 ** return (radio to main) value, the lower nibble is dontcare.
 **
 **/
#define IPC_MSG_ID_INC  0x50
#define IPC_MSG_ID_NEXT 0x51
#define IPC_MSG_ID_PREV 0x50
/// An updates badge_status payload.
#define IPC_MSG_STATS_UPDATE 0x70


typedef struct {
    uint16_t badge_id;
    uint8_t name[QC15_BADGE_NAME_LEN];
} ipc_msg_gd_arr_t;

//extern uint8_t ipc_state;
extern volatile uint8_t f_ipc_rx;

void ipc_init();
uint8_t ipc_tx(uint8_t *tx_buf, uint8_t len);
uint8_t ipc_tx_byte(uint8_t tx_byte);
uint8_t ipc_tx_op_buf(uint8_t op, uint8_t *tx_buf, uint8_t len);
uint8_t ipc_get_rx(uint8_t *rx_buf);

#endif /* IPC_H_ */
