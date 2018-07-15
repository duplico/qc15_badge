/// Main driver file for the Queercon 15 (2018) electronic badge.
/**
 ** This is for the main MCU on the badge, which is an MSP430FR5972. This file
 ** handles all of the definitions of the following types of data:
 **
 ** * Interrupt flags (which start with `f_`)
 ** * Non-interrupt signal flags (which start with `s_`).
 ** * Our global status and configuration
 **
 ** And it implements the following types of functions:
 **
 ** * Initialization of MCU-local peripherals and components (e.g. clocks,
 **    timers, GPIO for buttons, etc.
 ** * Signal-handling to convert low-level signals and interrupt flags (set by
 **    interrupt service routines, for example) into higher-level signals to be
 **    interpreted by the game functions.
 ** * The `main()` function, which calls all the initialization, bootstrapping,
 **    and power-on self-test functions. It also implements the main loop,
 **    which handles signals and calls other modules' loop body functions as
 **    appropriate.
 **
 ** \file main.c
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <msp430fr5972.h>
#include <driverlib.h>
#include <s25flash.h>

#include "qc15.h"

#include "lcd111.h"
#include "ht16d35b.h"
#include "s25flash.h"
#include "ipc.h"
#include "leds.h"
#include "game.h"

#include "util.h"
#include "main_bootstrap.h"

volatile uint8_t f_time_loop = 0;
uint8_t s_clock_tick = 0;
uint8_t s_buttons = 0;
uint8_t s_down = 0;
uint8_t s_up = 0;
uint8_t s_left = 0;
uint8_t s_right = 0;
uint8_t s_power_on = 0;
uint8_t s_power_off = 0;
qc15status badge_status = {0}; // TODO initialize elsewhere

uint8_t rx_from_radio[IPC_MSG_LEN_MAX] = {0};

const rgbcolor_t bw_colors[] = {
        {0x20, 0x20, 0x20},
        {0xff, 0xff, 0xff},
        {0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00},
};

const led_ring_animation_t anim_bw = {
        &bw_colors[0],
        4,
        10,
        "bwtest"
};

/// Initialize the system clocks and clock sources.
/**
 ** CLOCK SOURCES
 ** =============
 **
 ** Fixed sources:
 ** * `VLO`: 10k Very Low-power low-frequency Oscillator
 ** * `MODOSC`: A 5MHz clock for `MODCLK`
 ** * `LFMODCLK`: A divided version of `MODCLK` (/128), which is 39 kHz
 **
 ** Configurable sources:
 ** * `DCO` (Digitally-controlled oscillator): initialize to 8 MHz
 ** * `LFXT` (Low frequency external crystal): unused
 ** * `HFXT` (High frequency external crystal): unused
 **
 ** SYSTEM CLOCKS
 ** =============
 ** * `MCLK`:  Initialize to `DCO`/1, which is 8 MHz.
 ** * `SMCLK`: Initialize to `DCO`/8, which is 1 MHz, the same as `SMCLK` on
 **            the radio MCU.
 ** * `ACLK`:  Initialize to `LFMODOSC`, which is 39 kHz.
 */
void init_clocks() {

    // SYSTEM CLOCKS
    // =============
    // MCLK (8 MHz)
    //  Defaults to DCOCLK /8
    //  Available sources are HFXT, DCO, LFXT, VLO, or external digital clock.
    //   If it's above 8 MHz, we need to configure FRAM wait-states.
    //   Set to 8 MHz (DCO /1)
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // SMCLK (1 MHz)
    //  Defaults to DCOCLK /8
    //  Same sources available as MCLK.
    //      NB: This is different from the SMCLK behavior of the FR2xxx series,
    //          which can only source SMCLK from a divided MCLK.
    //  We'll use DCO /8 to get 1 MHz
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_8); // 1 M

    // MODCLK (5 MHz)
    //  This comes from MODOSC. It's fixed.

    // ACLK (39 kHz)
    //  Uses LFMODOSC, which is ~ 39k (MODCLK /128).
    CS_initClockSignal(CS_ACLK, CS_LFMODOSC_SELECT, CS_CLOCK_DIVIDER_1); // 39k
}

void init_ipc_io() {
    // IPC:
    // 2.0 A0_TX
    // 2.1 A0_RX
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,
                                                GPIO_PIN0+GPIO_PIN1,
                                                GPIO_PRIMARY_MODULE_FUNCTION);
    UCA0CTLW0 |= UCSWRST;
}

/// Unlock the IO pins from LPM5, and initialize our GPIO.
void init_io() {
    // The magic make-it-work command.
    //  Otherwise everything is stuck in high-impedance forever.
    PMM_unlockLPM5();

    lcd111_init_io();
    ht16d_init_io();
    s25flash_init_io();
    init_ipc_io();

    // Screw post inputs with pull-ups
    P7DIR &= ~(GPIO_PIN2+GPIO_PIN3+GPIO_PIN4); // inputs
    P7REN |= GPIO_PIN2+GPIO_PIN3+GPIO_PIN4; // resistor enable
    P7OUT |= GPIO_PIN2+GPIO_PIN3+GPIO_PIN4; // pull-up resistor

    // Buttons: 9.4, 9.5, 9.6, 9.7
    P9DIR &= 0x0F; // inputs
    P9REN |= 0xF0; // resistors on
    P9OUT |= 0xF0; // pull ups, please
}

// TODO: This needs to be changed to 32 Hz, to match the radio MCU
/// Initialize the animation timer to about 30 Hz
void timer_init() {
    // We need timer A3 for our loop below.
    Timer_A_initUpModeParam timer_param = {0};
    timer_param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK; // 1 MHz
    // We want this to go every 33 1/3 ms, so at 33 1/3 Hz
    //  (every 3,333 ticks @ 1MHz)
    timer_param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1; // /1
    timer_param.timerPeriod = 3333;
    timer_param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    timer_param.captureCompareInterruptEnable_CCR0_CCIE =
            TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE;
    timer_param.timerClear = TIMER_A_SKIP_CLEAR;
    timer_param.startTimer = false;
    Timer_A_initUpMode(TIMER_A1_BASE, &timer_param);
    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
}

/// The master init function, which calls IO and peripherals' init functions.
void init() {
    WDT_A_hold(WDT_A_BASE);

    init_clocks();
    init_io();

    lcd111_init();
    ht16d_init();
    s25flash_init();
    ipc_init();
    timer_init();

    // We're storing at least one compute-intensive function in a special
    //  place in FRAM, and then copying it to RAM, where it will run
    //  with lower power consumption, and possible faster, when invoked.
    // Copy FRAM_EXECUTE to RAM_EXECUTE
    memcpy((void *)0x1C00,(const void*)0x10000,0x0200);

    srand(25);
}

/// Debounce the buttons by checking for consecutive similar values.
/**
 ** This function is a highly silly way to debounce four buttons. It takes
 ** "advantage" of their being sequential on port P9. Because buttons 1, 2, 3,
 ** and 4 are attached to port P9 pins 7, 6, 5, and 4 respectively, we just
 ** mask the upper nibble of `P9IN` and use that for our current `button_read`,
 ** which we check against the last read, `button_read_prev`. If they're the
 ** same, we consider that to be a stable (debounced) button reading. Then we
 ** determine whether we need to raise a signal to the main function by
 ** comparing that to the current button state, which is in the upper nibble of
 ** `button_state`.
 **
 ** If we do need to raise a signal, we use `s_button` for the
 ** purpose: a 1 in its lower nibble indicates which button is being signaled,
 ** and a corresponding 1 in the upper nibble indicates a "release" whereas
 ** a 0 indicates a "press."
 */
void poll_buttons() {
    // The buttons are active LOW.
    static uint8_t button_read_prev = 0xF0;
    static uint8_t button_read = 0xF0;
    static uint8_t button_state = 0xF0;

    // The following is a very silly way to debounce 4 buttons.

    // Buttons 1,2,3,4 are attached to P9.7,P9.6,P9.5,P9.4 respectively.

    button_read = P9IN & 0xF0;
    for (uint8_t i=0; i<4; i++) {
        // For each of our 4 buttons:
        if ((button_read & (BIT4<<i)) == (button_read_prev & (BIT4<<i))
                && (button_read & (BIT4<<i)) != (button_state & (BIT4<<i))) {
            button_state &= ~(BIT4<<i);
            button_state |= (button_read & (BIT4<<i));

            // Flag the button. Lower nibble is WHICH button:
            s_buttons |= (BIT0<<i);

            // 0=down; 1=right; 2=left; 3=up

            // Upper nibble is its current value:
            //  (so if it's 1, it was just RELEASED,
            //    & if it's 0, it was just PRESSED.)
            s_buttons |= (button_read & (BIT4<<i));
        }
    }
    button_read_prev = button_read;
}

/// High-level message handler for IPC messages from the radio MCU.
void handle_ipc_rx(uint8_t *rx) {
    switch(rx[0] & 0xF0) {
    case IPC_MSG_POST:
        // The radio MCU has rebooted.
        // fall through:
    case IPC_MSG_STATS_REQ:
        // We need to prep and send a stats message for the radio.
        ipc_tx_op_buf(IPC_MSG_STATS_ANS, (uint8_t *) &badge_status, sizeof(qc15status));
        break;
    case IPC_MSG_SWITCH:
        // The switch has been toggled.
        if (rx[0] & 0x0F) {
            s_power_off = 0;
            s_power_on = 1;
        } else {
            s_power_on = 0;
            s_power_off = 1;
        }
        break;
    }
}

/// Low-level handler for global interrupt and status signals.
/**
 ** This function is ALWAYS called in the main loop, no matter what other
 ** loop bodies need to be called. It handles basic LED tasks, button polling,
 ** IPC messaging, the soft power switch, and interpreting the low-level
 ** button signals into four individual signals for other loop bodies.
 */
void handle_global_signals() {
    if (f_time_loop) {
        f_time_loop = 0;
        s_clock_tick = 1;
        led_timestep();
        poll_buttons();
    }

    if (f_ipc_rx) {
        f_ipc_rx = 0;
        if (ipc_get_rx(rx_from_radio)) {
            handle_ipc_rx(rx_from_radio);
        }
    }

    if (s_power_off) {
        s_power_off = 0;
        led_off();
    }

    if (s_power_on) {
        s_power_on = 0;
        led_on();
    }

    if (s_buttons) {
        if (s_buttons & BIT0) { // DOWN
            if (s_buttons & BIT4) {
                s_down = 1;
                // release
            } else {
                // press
            }
        }

        if (s_buttons & BIT1) { // RIGHT
            if (s_buttons & BIT5) {
                // release
                s_right = 1;
            } else {
                // press
            }
        }
        if (s_buttons & BIT2) { // LEFT
            if (s_buttons & BIT6) {
                // release
                s_left = 1;
            } else {
                // press
            }
        }
        if (s_buttons & BIT3) { // UP
            if (s_buttons & BIT7) {
                // release
                s_up = 1;
            } else {
                // press
            }
        }
        s_buttons = 0;
    }
}

/// Clear any loop-iteration-specific signals not handled by a loop body.
/**
 ** Specifically, this function cleans up after `handle_global_signals()`. Only
 ** signals set in that function are cleared here. Other signals may be
 ** important to persist across loops (especially interrupt flags) because they
 ** could have occurred between the invocations of `handle_global_signals()`
 ** and this function.
 */
void cleanup_global_signals() {
    s_left = 0;
    s_right = 0;
    s_down = 0;
    s_up = 0;
    s_clock_tick = 0;
}

/// The main initialization and loop function.
void main (void)
{
    init();

    __bis_SR_register(GIE);

    // hold DOWN on turn-on for verbose boot:
    bootstrap(P9IN & BIT4);

    game_begin();

    while (1) {
        handle_global_signals();

        // Handle a completed LED animation.
        if (s_led_anim_done) {
            s_led_anim_done = 0;
        }

        game_handle_loop();

        // If no further interrupt flags have been raised during this loop
        //  iteration, go to sleep. Otherwise, loop.
        if (!f_ipc_rx && !f_time_loop)
            LPM;
    }
}

/// The time loop ISR, at vector `0xFFDE` (`Timer1_A3 CC0`).
#pragma vector=TIMER1_A0_VECTOR
__interrupt
void TIMER_ISR() {
    // All we have here is CCIFG0
    f_time_loop = 1;
    LPM_EXIT;
}
