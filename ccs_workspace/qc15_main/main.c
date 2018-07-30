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
#include <stdio.h>

#include <msp430fr5972.h>
#include <driverlib.h>
#include <s25fs.h>
#include <s25fs.h>
#include <textentry.h>
#include "qc15.h"

#include "lcd111.h"
#include "ht16d35b.h"
#include "ipc.h"
#include "leds.h"
#include "game.h"
#include "util.h"
#include "main_bootstrap.h"
#include "flash_layout.h"
#include "badge.h"

// None of the following persist:
volatile uint8_t f_time_loop = 0;
uint8_t s_clock_tick = 0;
uint8_t s_buttons = 0;
uint8_t s_down = 0;
uint8_t s_up = 0;
uint8_t s_left = 0;
uint8_t s_right = 0;
uint8_t s_power_on = 0;
uint8_t s_power_off = 0;
uint8_t s_got_next_id = 0;
uint8_t s_gd_success = 0;
uint8_t s_gd_failure = 0;
uint8_t s_game_checkname_success = 0;

uint16_t gd_curr_id = 0;
uint16_t gd_starting_id = 0;

// Not persist
uint8_t power_switch_status = 0;

// Not persist
uint8_t qc15_mode;

// Not persist:
volatile qc_clock_t qc_clock;
uint16_t badges_nearby = 0;

// If I change this to NOINIT, it'll persist between flashings of the badge.
#pragma PERSISTENT(badge_conf)
qc15conf badge_conf = {0};
#pragma PERSISTENT(backup_conf)
qc15conf backup_conf = {0};

uint8_t global_flash_lockout = 0;

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
    // IF YOU CHANGE THIS, YOU **MUST** CHANGE MCLK_FREQ_KHZ IN qc15.h!!!
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // SMCLK (1 MHz)
    //  Defaults to DCOCLK /8
    //  Same sources available as MCLK.
    //      NB: This is different from the SMCLK behavior of the FR2xxx series,
    //          which can only source SMCLK from a divided MCLK.
    //  We'll use DCO /8 to get 1 MHz
    // IF YOU CHANGE THIS, YOU **MUST** CHANGE SMCLK_FREQ_KHZ IN qc15.h!!!
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
    s25fs_init_io();
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

/// Initialize the animation timer to tick every 1/32 of a second.
void timer_init() {
    // We need timer A3 for our loop below.
    Timer_A_initUpModeParam timer_param = {0};
    timer_param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK; // 1 MHz
    // We want this to go every 1/32 of a second (32 Hz).
    //  (Every 31250 ticks @ 1 MHz)
    timer_param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1; // /1
    timer_param.timerPeriod = 31250;
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

    // We're storing at least one compute-intensive function in a special
    //  place in FRAM, and then copying it to RAM, where it will run
    //  with lower power consumption, and possible faster, when invoked.
    // Copy FRAM_EXECUTE to RAM_EXECUTE
    memcpy((void *)0x1C00,(const void*)0x10000,0x0200);

    init_clocks();
    init_io();

    ht16d_init();
    lcd111_init();
    s25fs_init();
    ipc_init();
    timer_init();
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
    // Grab the payload, since rx[0] is an opcode.
    switch(rx[0] & 0xF0) {
    case IPC_MSG_POST:
        // The radio MCU has rebooted.
        // fall through:
    case IPC_MSG_STATS_REQ:
        // We need to prep and send a stats message for the radio.
        // Because badge_status is a subset of badge_conf that appears at
        //  its beginning, that's what we copy from.
        ipc_tx_op_buf(IPC_MSG_STATS_UPDATE, (uint8_t *) &badge_conf, sizeof(qc15status));
        break;
    case IPC_MSG_SWITCH:
        // The switch has been toggled.
        if (rx[0] & 0x0F) {
            s_power_off = 0;
            s_power_on = 1;
            power_switch_status = POWER_SW_ON;
        } else {
            s_power_on = 0;
            s_power_off = 1;
            power_switch_status = POWER_SW_OFF;
        }
        break;
    case IPC_MSG_GD_ARR:
        // Someone has arrived
        set_badge_seen(
                rx[1] + ((uint16_t)rx[2] << 8),
                &(rx[3])
        );
        if (badges_nearby < 450)
            badges_nearby++;
        break;
    case IPC_MSG_GD_DEP:
        // Someone has departed.
        if (badges_nearby)
            badges_nearby--;
        break;
    case IPC_MSG_GD_DL:
        // We successfully downloaded from a badge
        if (rx[0] & 0x0F) {
            s_gd_success = 1;
        } else {
            s_gd_failure = 1;
        }
        break;
    case IPC_MSG_GD_UL:
        // Someone downloaded from us.
        set_badge_uploaded((uint16_t)rx[1] + ((uint16_t)rx[2] << 8));
        break;
    case IPC_MSG_ID_INC:
        // We got the ID we asked for.
        s_got_next_id = 1;
        gd_curr_id = rx[1] + ((uint16_t)rx[2] << 8);
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
    uint8_t rx_from_radio[IPC_MSG_LEN_MAX];

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

    if (s_power_off && power_switch_status == POWER_SW_OFF) {
        s_power_off = 0;
        led_off();
    }

    if (s_power_on && power_switch_status == POWER_SW_ON) {
        s_power_on = 0;
        led_on();
    }

    if (s_buttons) {
        if (s_buttons & BIT0) { // DOWN
            if (s_buttons & BIT4)
                s_down = 1; // release
        }
        if (s_buttons & BIT1) { // RIGHT
            if (s_buttons & BIT5)
                s_right = 1; // release
        }
        if (s_buttons & BIT2) { // LEFT
            if (s_buttons & BIT6)
                s_left = 1; // release
        }
        if (s_buttons & BIT3) { // UP
            if (s_buttons & BIT7)
                s_up = 1; // release
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

/// Initialize the badge's running state to a known good one.
void badge_startup() {
    // Handle our main config
    init_config();
    badge_conf.active = 1;
    save_config(); // TODO: just send it to the radio
    // Handle our animation
    // Handle our code LEDs
    // Handle entering the proper state
    qc15_mode = QC15_MODE_GAME;
    game_begin();
}

/// Handle the inner loop of the mode where we're searching for a named badge.
void checkname_handle_loop() {
    // This function is limited to 450 calls total, just in case we encounter a
    //  race condition.
    static uint16_t calls = 0;

    char curr_name[QC15_PERSON_NAME_LEN];

    if (s_got_next_id) {
        // We received the next ID from the radio MCU
        s_got_next_id = 0;

        // First, handle our first call to set up our base case.
        if (gd_starting_id == GAME_NULL) {
            calls = 0;
            // We got our first result from the radio module. Store it.
            // We'll be looking for this, or a value below it, or null,
            //  in order to know that we've looped around.
            gd_starting_id = gd_curr_id;
        }

        // Increment our guard.
        calls++;

        if (gd_curr_id == GAME_NULL) {
            // This indicates nobody's around.
            // No joy. Tell the game we failed.
            qc15_mode = QC15_MODE_GAME;
            s_game_checkname_success = 0;
            return;
        }

        // We know the ID isn't null, and we know we've seen it. What's its
        //  name?
        load_person_name(curr_name, gd_curr_id);

        // Is this the name we're looking for?
        if (!strcmp(curr_name, game_name_buffer)) {
            // We found the name we're looking for. Hooray!
            s_game_checkname_success = 1;
            qc15_mode = QC15_MODE_GAME;
            return;
        }

        if (gd_curr_id <= gd_starting_id || calls >= QC15_BADGES_IN_SYSTEM) {
            // We're done, and we haven't found the name we're looking for.
            qc15_mode = QC15_MODE_GAME;
            s_game_checkname_success = 0;
            return;
        }

        // If we're down here, we're still looking, but we also haven't found
        //  the name yet. So ask the radio MCU for the next ID.
        while (!ipc_tx_op_buf(IPC_MSG_ID_NEXT, &gd_curr_id, 2));
    }
}

void connect_handle_loop() {
    char curr_name[QC15_PERSON_NAME_LEN];
    char text[25];

    static uint8_t waiting_for_radio = 1;

    if (s_got_next_id) {
        // We received the next ID from the radio MCU
        s_got_next_id = 0;
        waiting_for_radio = 0;
        if (gd_curr_id == GAME_NULL) {
            // Nothing. Nobody's nearby, we have to show the EXIT option.
            lcd111_set_text(LCD_TOP, "Nobody is detected.");
            draw_text(LCD_BTM, "Cancel", 0);
        } else {
            // It's a REAL ONE!
            // TODO: Do we need a gaydar.c/h module?
            load_badge_name(curr_name, gd_curr_id);
            sprintf(text, "Badge 0x%x:%s", gd_curr_id, curr_name);
            draw_text(LCD_BTM, text, 1);
            load_person_name(curr_name, gd_curr_id);
            sprintf(text, "Holder: %s", curr_name);
            draw_text(LCD_TOP, text, 1);
        }
    }

    if (s_gd_success || s_gd_failure) {
        waiting_for_radio = 0;
        if (set_badge_downloaded(gd_curr_id)) {
            // New!
            s_gd_success = 2;
        } else {
            // Old.
        }

        // We specifically do NOT clear these signals here.
        qc15_mode = QC15_MODE_GAME;
    }

    if (waiting_for_radio) {
        // User input is BLOCKED OUT while we're talking to the other MCU.
        // TODO: Consider a timeout.
        return;
    }

    if (qc_clock.time % 512 == 0) {
        // Every 16 seconds,
        // We need to send an advertisement.
        while (!ipc_tx_byte(IPC_MSG_GD_EN));
    }

    if (s_up || s_down) {
        lcd111_set_text(LCD_BTM, "");
        waiting_for_radio = 1;
        while (!ipc_tx_op_buf(
                s_down? IPC_MSG_ID_NEXT : IPC_MSG_ID_PREV,
                &gd_curr_id,
                2)
        );
    } else if (s_right) {
        // Try to connect!
        waiting_for_radio = 1;
        while (!ipc_tx_op_buf(
                IPC_MSG_GD_DL,
                &gd_curr_id,
                2)
        );
        // Now, the IPC functions will trigger either an IPC
        //  s_gd_failure or s_gd_success. Once we detect one of those,
        //  we release control back to the game logic.
    }

}

/// The main initialization and loop function.
void main (void)
{
    init();

    uint8_t initial_buttons = P9IN;

    // hold RIGHT button at startup for flash mode.
    if (!(initial_buttons & BIT5))
        flash_bootstrap();

    __bis_SR_register(GIE);

    badge_conf.active = 0;
    // hold DOWN on turn-on for verbose boot:
    bootstrap(initial_buttons & BIT4); // interrupts required.

    // Housekeeping is now concluded. It's time to see the wizard.
    badge_startup();

    while (1) {
        handle_global_signals();

        // Handle a completed LED animation.
        if (s_led_anim_done) {
            s_led_anim_done = 0;
        }

        switch(qc15_mode) {
        case QC15_MODE_COUNTDOWN:
            break;
        case QC15_MODE_SLEEP:
            break;
        case QC15_MODE_STATUS:
            break;
        case QC15_MODE_GAME:
            game_handle_loop();
            break;
        case QC15_MODE_TEXTENTRY:
            textentry_handle_loop();
            break;
        case QC15_MODE_GAME_CHECKNAME:
            // What we want here is MOSTLY going to be handled in the
            //  ipc loop. We're starting at id 0, and
            checkname_handle_loop();
            break;
        case QC15_MODE_GAME_CONNECT:
            connect_handle_loop();
            break;
        }

        cleanup_global_signals();

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
    qc_clock.time++;
    LPM_EXIT;
}
