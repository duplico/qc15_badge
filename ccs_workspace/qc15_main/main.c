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
#include "codes.h"
#include "led_animations.h"

#include "menu.h"

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
uint8_t s_turn_on_file_lights = 0;

uint16_t gd_curr_id = 0;
uint16_t gd_curr_connectable = 0;
uint16_t gd_starting_id = 0;

// Not persist
uint8_t power_switch_status = 0;

// Not persist
uint8_t qc15_mode;

// Not persist:
volatile qc_clock_t qc_clock;
uint16_t badges_nearby = 0;

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

void adc_init() {// Initialize the shared reference module
    // By default, REFMSTR=1 => REFCTL is used to configure the internal reference
    while(REFCTL0 & REFGENBUSY);              // If ref generator busy, WAIT
    REFCTL0 |= REFVSEL_0 + REFON;             // Enable internal 1.2V reference

    /* Initialize ADC12_A */
    ADC12CTL0 &= ~ADC12ENC;                   // Disable ADC12
    ADC12CTL0 = ADC12SHT0_8 + ADC12ON;        // Set sample time
    ADC12CTL1 = ADC12SHP;                     // Enable sample timer
    ADC12CTL3 = ADC12TCMAP;                   // Enable internal temperature sensor
    ADC12MCTL0 = ADC12VRSEL_1 + ADC12INCH_30; // ADC input ch A30 => temp sense

    while(!(REFCTL0 & REFGENRDY));            // Wait for reference generator
                                              // to settle
    ADC12CTL0 |= ADC12ENC;

    ADC12CTL0 |= ADC12SC;                   // Sampling and conversion start
}

/// The master init function, which calls IO and peripherals' init functions.
void init() {
    WDT_A_hold(WDT_A_BASE);

    init_clocks();
    init_io();

    ht16d_init();
    lcd111_init();
    s25fs_init();
    ipc_init();
    timer_init();
    adc_init();
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
    uint16_t id;
    qc_clock_t temp_clock;

    // Grab the payload, since rx[0] is an opcode.
    switch(rx[0] & 0xF0) {
    case IPC_MSG_POST:
        // The radio MCU has rebooted.
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
        id = rx[1] + ((uint16_t)rx[2] << 8);
        set_badge_seen(
                id,
                &(rx[3])
        );
        if (id < QC15_BADGES_IN_SYSTEM && badges_nearby < QC15_BADGES_IN_SYSTEM)
            badges_nearby++;
        break;
    case IPC_MSG_GD_DEP:
        // Someone has departed.
        id = rx[1] + ((uint16_t)rx[2] << 8);
        if (id < QC15_BADGES_IN_SYSTEM && badges_nearby)
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
        if (rx[0] & IPC_MSG_ID_CONNECTABLE)
            gd_curr_connectable = 1;
        else
            gd_curr_connectable = 0;
        break;
    case IPC_MSG_TIME_UPDATE:
        memcpy((uint8_t *)&temp_clock, &rx[1], sizeof(qc_clock_t));
        qc_clock.time = temp_clock.time;
        qc_clock.authoritative = temp_clock.authoritative;
        break;
    case IPC_MSG_CALIBRATE_FREQ:
        badge_conf.freq_set = 1;
        badge_conf.freq_center = rx[1];
        save_config(0);
        // WDT hold
        WDT_A_hold(WDT_A_BASE);
        ht16d_all_one_color_ring_only(0x00, 0x8F, 0x00);
        delay_millis(2000);
        // WDT unhold
        ht16d_all_one_color_ring_only(0x00, 0x00, 0x00);
        WDTCTL = WDTPW | WDTSSEL__ACLK | WDTIS__32K;
        break;
    }
}

void poll_temp() {
    volatile uint16_t temp_c;
    uint16_t voltage_at_30c = *((unsigned int *)0x1A1A);
    uint16_t voltage_at_85c = *((unsigned int *)0x1A1C);

    if (!(ADC12IFGR0 & ADC12IFG0))
        return;

    temp_c = (((long)ADC12MEM0 - voltage_at_30c) * (85 - 30))
             / (voltage_at_85c - voltage_at_30c)
             +  30;

    if (temp_c < 10) {
        badge_conf.freezer_done = 1;
        led_set_anim(&all_animations[FLAG_FREEZER], 0, 0xFF, 0);
        unlock_flag(FLAG_FREEZER);
        decode_event(EVENT_FREEZER); // this also saves right before returning.
    }

    if (qc15_mode == QC15_MODE_TEMP) {
        char text[25];
        sprintf(text, "T: %d", temp_c);
        lcd111_set_text(LCD_TOP, text);
    }

    // Sample again.
    ADC12CTL0 |= ADC12SC;
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
    static uint8_t up_status = 0;

    if (s_turn_on_file_lights) {
        led_activate_file_lights();
        s_turn_on_file_lights = 0;
    }

    if (f_time_loop) {
        // pat pat pat
        WDT_A_resetTimer(WDT_A_BASE);
        f_time_loop = 0;
        s_clock_tick = 1;
        led_timestep();
        poll_buttons();
        if (!badge_conf.freezer_done && !(qc_clock.time & 0xFF))
            poll_temp(); // every 8 seconds, poll the temp.

        if (badge_conf.event_beacon && qc_clock.time > disable_event_at) {
            badge_conf.event_beacon = 0;
            save_config(1);
        }

        if (!flag_unlocked(FLAG_RAINBOW) && qc_clock.time > QC_PARTY_TIME) {
            unlock_flag(FLAG_RAINBOW);
            led_set_anim(&all_animations[FLAG_RAINBOW],
                         0, 0xff, 0);
        }

    }

    if (f_ipc_rx) {
        f_ipc_rx = 0;
        if (ipc_get_rx(rx_from_radio)) {
            handle_ipc_rx(rx_from_radio);
        }
    }

    if (s_power_off && power_switch_status == POWER_SW_OFF) {
        s_power_off = 0;
        qc15_set_mode(QC15_MODE_SLEEP);
    }

    if (s_power_on && power_switch_status == POWER_SW_ON) {
        s_power_on = 0;
        led_on();
        if (badge_conf.countdown_over)
            qc15_set_mode(QC15_MODE_GAME);
        else
            qc15_set_mode(QC15_MODE_COUNTDOWN);
        if (up_status && (
                is_handler(badge_conf.badge_id) ||
                badge_conf.badge_id <= 1)) {
            menu_suppress_click = 1;
            qc15_set_mode(QC15_MODE_CONTROLLER);
        } else if (up_status) {
            // non-special people, it goes to the status menu.
            menu_suppress_click = 1;
            qc15_set_mode(QC15_MODE_STATUS);
        }
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
            if (s_buttons & BIT7) {
                s_up = 1; // release
                up_status = 0;
            }
            else {
                up_status = 1;
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

void qc15_set_mode(uint8_t mode) {
    if (qc15_mode == QC15_MODE_SLEEP) {
        // TODO: turn stuff on.
    }

    switch(mode) {
    case QC15_MODE_COUNTDOWN:
        // This one is fine by itself.
        break;
    case QC15_MODE_SLEEP:
        // TODO: turn everything off.
        break;
    case QC15_MODE_STATUS:
        enter_menu_status();
        status_render_choice();
        break;
    case QC15_MODE_GAME:
        // Clear top screen
        // Render bottom screen.
        game_render_current();
        break;
    case QC15_MODE_TEXTENTRY:
        // This is entirely handled by textentry_begin()
        break;
    case QC15_MODE_GAME_CHECKNAME:
        // This is entirely handled by the game and checkname engine.
        break;
    case QC15_MODE_GAME_CONNECT:
        // This is entirely handled by the game and checkname engine.
        break;
    case QC15_MODE_TEMP:
        // This is handled by the time loop.
        break;
    case QC15_MODE_FLASH_BROKEN:
        led_set_anim(&anim_rainbow_spin, LED_ANIM_TYPE_NONE,
                     0xFF, led_ring_anim_pad_loops_bg);
        lcd111_set_text(LCD_TOP, "    Q U E E R C O N");
        lcd111_set_text(LCD_BTM, "     F I F T E E N");
        break;
    case QC15_MODE_CONTROLLER:
        enter_menu_controller();
        control_render_choice();
        break;
    }
    qc15_mode = mode;
}

/// Initialize the badge's running state to a known good one.
void badge_startup() {
    if (global_flash_lockout & FLASH_LOCKOUT_READ) {
        qc15_set_mode(QC15_MODE_FLASH_BROKEN);
        return;
    }
    // Handle our main config
    init_config();

    // Setup our initial time.
    qc_clock.time = badge_conf.last_clock;
    qc_clock.authoritative = 0;
    qc_clock.fault = 0;

    // All badges' names have guaranteed null-terms because they're
    //  declared explicitly.

    // Guarantee null-terms on person names:
    for (uint16_t i=0; i<QC15_BADGES_IN_SYSTEM; i++) {
        person_names[i][QC15_BADGE_NAME_LEN-1] = 0x00;
        if (person_names[i][0] == 0xff || person_names[i][0] == 0x00) {
            // bad name:
            person_names[i][0] = '?';
            person_names[i][1] = 0x00;
        }
    }

    badge_conf.active = 1;
    unlock_radio_status = 1;
    badge_conf.event_beacon = 0;
    save_config(1); // Recompute CRC for active=1, and tell the radio.
                   // This is the VERY FIRST MESSAGE we will send the radio.

    // Handle our animations:
    if (led_anim_type_bg != LED_ANIM_TYPE_NONE) {
        led_set_anim(led_ring_anim_bg, LED_ANIM_TYPE_NONE,
                     0xFF, led_ring_anim_pad_loops_bg);
    }

    if (badge_conf.file_lights_on) {
        led_activate_file_lights();
    }

    if (game_curr_state_id == STATE_ID_FIRSTBOOTCONFUSED) {
        unlock_flag(19);
        led_set_anim(&all_animations[19], 0,
                     0xFF, 0);
    }

    // Handle entering the proper state
    if (badge_conf.countdown_over) {
        qc15_set_mode(QC15_MODE_GAME);
        game_begin();
    }
    else {
        qc15_set_mode(QC15_MODE_COUNTDOWN);
    }
}

/// Handle the inner loop of the mode where we're searching for a named badge.
void checkname_handle_loop() {
    // This function is limited to 450 calls total, just in case we encounter a
    //  race condition.
    static uint16_t calls = 0;

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
        // Is this the name we're looking for?
        if (!strcmp("QUEERCON", game_name_buffer)) {
            s_game_checkname_success = 1;
            qc15_mode = QC15_MODE_GAME;
            return;
        } else if (!strcmp(person_names[gd_curr_id], game_name_buffer)){
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
        while (!ipc_tx_op_buf(IPC_MSG_ID_NEXT, (uint8_t *)&gd_curr_id, 2));
    }
}

void connect_handle_loop() {
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
            if (gd_curr_connectable) {
                sprintf(text, "[\xA4] 0x%x:%s", gd_curr_id, badge_names[gd_curr_id]);
            } else {
                sprintf(text, "[ ] 0x%x:%s", gd_curr_id, badge_names[gd_curr_id]);
            }
            draw_text(LCD_BTM, text, 1);
            sprintf(text, "Holder: %s", person_names[gd_curr_id]);
            draw_text(LCD_TOP, text, 1);
        }
    }

    if (s_gd_success || s_gd_failure) {
        waiting_for_radio = 0;
    }

    if (waiting_for_radio) {
        // User input is BLOCKED OUT while we're talking to the other MCU.
        return;
    }

    if (s_gd_success) {
        if (set_badge_downloaded(gd_curr_id)) {
            // New!
            s_gd_success = 2;
        } else {
            // Old.
            s_gd_success = 1;
        }

        // We specifically do NOT clear these signals here.
        qc15_set_mode(QC15_MODE_GAME);
        return;
    } else if (s_gd_failure) {
        qc15_set_mode(QC15_MODE_GAME);
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
                (uint8_t *)&gd_curr_id,
                2)
        );
    } else if (s_right) {
        // Try to connect!
        waiting_for_radio = 1;
        while (!ipc_tx_op_buf(
                IPC_MSG_GD_DL,
                (uint8_t *)&gd_curr_id,
                2)
        );
        // Now, the IPC functions will trigger either an IPC
        //  s_gd_failure or s_gd_success. Once we detect one of those,
        //  we release control back to the game logic.
    }

}

void countdown_handle_loop() {
    uint32_t countdown;
    char text[25] = "";

    if (!s_clock_tick)
        return;

    if (qc_clock.time >= QC_START_TIME) {
        // QUEERCON TIME!!!!!!
        badge_conf.countdown_over = 1;
        led_set_anim_none();
        led_set_anim(&anim_countdown_done, 0, 0, 0);
        save_config(0);
        qc15_set_mode(QC15_MODE_GAME);
        game_begin();
        return;
    }

    if (qc_clock.time % 54 == 0) {
        led_set_anim(&anim_countdown_tick, 0, 0, 0);
        save_config(0); // save our clock.
    }

    countdown = QC_START_TIME - qc_clock.time;
    sprintf(text, "       0x0%x.%x", (uint16_t)((0xffff0000 & countdown) >> 16),
                          (uint16_t)(0x0000ffff & countdown));
    lcd111_set_text(LCD_TOP, text);
    lcd111_set_text(LCD_BTM, text);
}

// We need the following:
//  We HAVE TO have an initial clock before enabling interrupts, and it
//  NEEDS to be right.

/// The main initialization and loop function.
void main (void)
{
    init();

    uint8_t initial_buttons = P9IN;

    // hold RIGHT button at startup for flash mode.
    if (!(initial_buttons & BIT5))
        flash_bootstrap();

    // Prevent the radio MCU from leaving POST until we want it to, by
    //  sending an active=1 update.
    badge_conf.active = 0;
    // Prevent our config save function from updating the radio like normal,
    //  until we actually have a sane config.
    unlock_radio_status = 0;

    __bis_SR_register(GIE);

    // hold DOWN on turn-on for verbose boot:
    bootstrap(initial_buttons & BIT4); // interrupts required.

    // Housekeeping is now concluded. It's time to see the wizard.
    // This function will set active and unlock_radio_status at the
    //  correct time:
    badge_startup();

    if (!(initial_buttons & BIT7)) {
        // UP:
        // Tell the radio MCU to calibrate its frequency.
        badge_conf.freq_center = 0;
        badge_conf.freq_set = 0;
        unlock_radio_status = 0;
        save_config(0);
        unlock_radio_status = 1;
        delay_millis(500);
        while (!ipc_tx_byte(IPC_MSG_CALIBRATE_FREQ));
    }

    WDTCTL = WDTPW | WDTSSEL__ACLK | WDTIS__32K;

    while (1) {
        handle_global_signals();

        // Handle a completed LED animation.
        if (s_led_anim_done) {
            s_led_anim_done = 0;
        }

        switch(qc15_mode) {
        case QC15_MODE_COUNTDOWN:
            countdown_handle_loop();
            break;
        case QC15_MODE_SLEEP:
            break;
        case QC15_MODE_STATUS:
            status_handle_loop();
            break;
        case QC15_MODE_GAME:
            game_handle_loop();
            break;
        case QC15_MODE_TEXTENTRY:
            textentry_handle_loop();
            break;
        case QC15_MODE_GAME_CHECKNAME:
            // A lot of this is handled in the handle ipc loop, too:
            checkname_handle_loop();
            break;
        case QC15_MODE_GAME_CONNECT:
            connect_handle_loop();
            break;
        case QC15_MODE_TEMP:
            __no_operation();
            break;
        case QC15_MODE_FLASH_BROKEN:
            __no_operation();
            break;
        case QC15_MODE_CONTROLLER:
            controller_handle_loop();
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
