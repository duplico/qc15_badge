/*
 * radio_bootstrap.c
 *
 *  Created on: Jul 14, 2018
 *      Author: george
 */

#include <stdint.h>

#include <msp430fr2422.h>

#include "radio.h"
#include "ipc.h"

#include "qc15.h"

extern volatile uint8_t f_time_loop;
extern uint8_t s_switch;
extern volatile uint64_t qc_clock;
extern uint8_t sw_state;
void poll_switch();

#define POST_MCU 0
#define POST_XT1 1
#define POST_RFM 2
#define POST_IPC 3
#define POST_IPC_OK 4
#define POST_OK 5

void bootstrap() {
    // We'll do our POST here, which involves:
    // 1. MCU
    // 2. Crystal
    // 3. Radio
    // 4. IPC
    uint8_t rx_from_main[IPC_MSG_LEN_MAX] = {0};
    uint8_t bootstrap_status = POST_MCU;
    uint8_t failure_flags = 0x00;
    qc_clock = 0;

    if (bootstrap_status == POST_MCU) {
        if (!1) {
            failure_flags |= BIT0; // General purpose flag.
        }
        bootstrap_status++;
    }

    if (bootstrap_status == POST_XT1) {
        if (CSCTL7 & XT1OFFG)
            failure_flags |= BIT1; // crystal fault
        bootstrap_status++;
    }

    if (bootstrap_status == POST_RFM) {
        if (!rfm75_post()) {
            // Radio failure:
            failure_flags |= BIT2;
        }
        bootstrap_status++;
    }

    ipc_tx_byte(IPC_MSG_POST | failure_flags);

    while (1) {
        if (f_time_loop) {
            f_time_loop = 0;
        }

        // We're only allowed to leave the bootstrap loop once we've received
        //  our startup configuration from the main MCU. If we never receive it,
        //  this means that either (a) the main MCU is broken, in which case it
        //  doesn't matter what we do, or (b) the IPC channel is broken, in
        //  which case there's no possible way for us to get useful behavior.
        // TODO: Disable the radio during bootstrap.

        if (f_ipc_rx) {
            f_ipc_rx = 0;
            if (ipc_get_rx(rx_from_main)) {
                if (rx_from_main[0] == IPC_MSG_STATS_ANS) {
                    // Read the current status into our volatile copy of it.
                    memcpy(&badge_status, &rx_from_main[1], sizeof(qc15status));

                    // POST/bootstrap process is done.
                    break;
                }
            }
        }

        // Re-send our POST message (which doubles as a request for our startup
        //  status/config struct) every quarter-second.
        if (bootstrap_status == POST_IPC && qc_clock==8) {
            qc_clock = 0;
            ipc_tx_byte(IPC_MSG_POST | failure_flags);
        }
    }
    // Bootstrapping is complete. Cleanup:
    qc_clock = 0;
    // TODO: re-enable the radio.
}
