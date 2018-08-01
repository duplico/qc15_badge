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

volatile uint8_t f_time_loop = 0;
uint8_t s_clock_tick = 0;
uint8_t s_buttons = 0;
uint8_t s_down = 0;
uint8_t s_up = 0;
uint8_t s_left = 0;
uint8_t s_right = 0;
uint8_t s_power_on = 0;
uint8_t s_power_off = 0;

uint8_t power_switch_status = 0;

uint8_t mode_countdown, mode_status, mode_game, mode_sleep;
// text_entry_in_progress

uint8_t tx_cnt = 0;
uint8_t rx_cnt = 0;

volatile uint32_t qc_clock;

uint16_t badges_nearby = 0;

// If I change this to NOINIT, it'll persist between flashings of the badge.
#pragma PERSISTENT(badge_conf)
qc15conf badge_conf = {0};
#pragma PERSISTENT(backup_conf)
qc15conf backup_conf = {0};

uint8_t out_of_bootstrap = 0;

const rgbcolor_t bw_colors[] = {
        {0x20, 0x20, 0x20},
        {0xff, 0xff, 0xff},
        {0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00},
};

const led_ring_animation_t anim_bw = (led_ring_animation_t){
        .colors=&bw_colors[0],
        .len = 4,
        .speed = 4,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME
};

const rgbcolor_t g_colors[] = {
        {0x00, 0x20, 0x00},
        {0x00, 0xff, 0x00},
        {0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00},
};

const led_ring_animation_t anim_g = (led_ring_animation_t){
        .colors=&g_colors[0],
        .len = 4,
        .speed = 4,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME
};

void set_badge_seen(uint16_t id, uint8_t *name);

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
        // rx = green
        if (out_of_bootstrap) {
            led_set_anim(&anim_g, LED_ANIM_TYPE_SAME, 0, 0);
            rx_cnt++;
        }
        break;
    case IPC_MSG_GD_DEP:
        // tx = white
        if (out_of_bootstrap) {
            led_set_anim(&anim_bw, LED_ANIM_TYPE_SAME, 0, 0);
            tx_cnt++;
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


/// Counts the bits set in all the bytes of a buffer and returns it.
/**
 ** This is the Brian Kernighan, Peter Wegner, and Derrick Lehmer way of
 ** counting bits in a bitstring. See _The C Programming Language_, 2nd Ed.,
 ** Exercise 2-9; or _CACM 3_ (1960), 322.
 */
uint16_t buffer_rank(uint8_t *buf, uint8_t len) {
    uint16_t count = 0;
    uint8_t c, v;
    for (uint8_t i=0; i<len; i++) {
        v = buf[i];
        for (c = 0; v; c++) {
            v &= v - 1; // clear the least significant bit set
        }
        count += c;
    }
    return count;
}

void save_config() {
    // We only save our config in FRAM; for now the flash is READ ONLY.
    crc16_append_buffer((uint8_t *) (&badge_conf), sizeof(qc15conf)-2);

    memcpy(&backup_conf, &badge_conf, sizeof(qc15conf));

    // And, update our friend the radio MCU:
    // (spin until the send is successful)
    while (!ipc_tx_op_buf(IPC_MSG_STATS_UPDATE, (uint8_t *) (uint8_t *) (&badge_conf), sizeof(qc15status)));

}

uint8_t is_handler(uint16_t id) {
    return ((id <= QC15_HANDLER_LAST) &&
            (id >= QC15_HANDLER_START));
}

uint8_t is_uber(uint16_t id) {
    return (id < QC15_UBER_COUNT);
}

uint8_t check_id_buf(uint16_t id, uint8_t *buf) {
    uint8_t byte;
    uint8_t bit;
    byte = id / 8;
    bit = id % 8;
    return (buf[byte] & (BIT0 << bit)) ? 1 : 0;
}

void set_id_buf(uint16_t id, uint8_t *buf) {
    uint8_t byte;
    uint8_t bit;
    byte = id / 8;
    bit = id % 8;
    buf[byte] |= (BIT0 << bit);
}

uint8_t badge_seen(uint16_t id) {
    return check_id_buf(id, badge_conf.badges_seen);
}

uint8_t badge_uploaded(uint16_t id) {
    return check_id_buf(id, badge_conf.badges_uploaded);
}

uint8_t badge_downloaded(uint16_t id) {
    return check_id_buf(id, badge_conf.badges_downloaded);
}

/*
 *
 ** READ NAMES:
 ** 0x100000 -   0 -  19 (220 bytes)
 ** 0x110000 -  20 -  39 (220 bytes)
 ** ...
 */

void set_badge_seen(uint16_t id, uint8_t *name) {
    if (id >= QC15_BADGES_IN_SYSTEM)
        return;
    if (badge_seen(id)) {
        return;
    }
    set_id_buf(id, badge_conf.badges_seen);
    badge_conf.badges_seen_count++;

    // ubers
    if (is_uber(id)) {
        badge_conf.ubers_seen |= (BIT0 << id);
        badge_conf.ubers_seen_count++;
    }
    // handlers
    if (is_handler(id)) {
        badge_conf.handlers_seen |= (BIT0 << (id - QC15_HANDLER_START));
        badge_conf.handlers_seen_count++;
    }

    uint32_t name_address =   0x100000;
    name_address += (id/20) * 0x010000;
    name_address += (id%20) * 11;

    // TODO: read 220 byte block, write name.

    save_config();
}

void set_badge_uploaded(uint16_t id) {
    if (id >= QC15_BADGES_IN_SYSTEM)
        return;
    if (badge_uploaded(id)) {
        return;
    }
    set_id_buf(id, badge_conf.badges_uploaded);
    badge_conf.badges_uploaded_count++;
    // ubers
    if (is_uber(id)) {
        badge_conf.ubers_uploaded |= (BIT0 << id);
        badge_conf.ubers_uploaded_count++;
    }
    // handlers
    if (is_handler(id)) {
        badge_conf.handlers_uploaded |= (BIT0 << (id - QC15_HANDLER_START));
        badge_conf.handlers_uploaded_count++;
    }

    save_config();
}

// TODO: Break out the save_config part
void set_badge_downloaded(uint16_t id) {
    if (id >= QC15_BADGES_IN_SYSTEM)
        return;
    if (badge_downloaded(id)) {
        return;
    }
    set_id_buf(id, badge_conf.badges_downloaded);
    badge_conf.badges_downloaded_count++;
    // ubers
    if (is_uber(id)) {
        badge_conf.ubers_downloaded |= (BIT0 << id);
        badge_conf.ubers_downloaded_count++;
    }
    // handlers
    if (is_handler(id)) {
        badge_conf.handlers_downloaded |= (BIT0 << (id - QC15_HANDLER_START));
        badge_conf.handlers_downloaded_count++;
    }

    save_config();
}

void generate_config() {
    // All we start from, here, is our ID.

    // The struct is no good. Zero it out.
    memset(&badge_conf, 0x00, sizeof(qc15conf));

    // Load ID from flash:
    // TODO: Confirm Endianness
    s25fs_read_data((uint8_t *)(&(badge_conf.badge_id)), FLASH_ADDR_ID_MAIN, 2);

    uint8_t sentinel;
    s25fs_read_data(&sentinel, FLASH_ADDR_sentinel, 1);

    // TODO: we need a global flag for "flash no workie".
    // Check the handful of things we know to check in the flash to see if
    //  it's looking sensible:
    if (sentinel != FLASH_sentinel_BYTE ||
            badge_conf.badge_id >= QC15_BADGES_IN_SYSTEM ||
            badge_conf.badge_name[0] == 0xFF)
    {
        badge_conf.badge_id = 111;
        char backup_name[] ="Skippy";
        strcpy(badge_conf.badge_name, backup_name);
    }

    // Determine which segment we have (and therefore which parts)
    badge_conf.code_starting_part = (badge_conf.badge_id % 16) * 6;
    uint8_t name[11] = "Human";
    set_badge_seen(badge_conf.badge_id, name);
    set_badge_uploaded(badge_conf.badge_id);
    set_badge_downloaded(badge_conf.badge_id);
    save_config();

    mode_game = 1;
    mode_countdown = 0;
    mode_status = 0;
    mode_sleep = 0;
}

uint8_t config_is_valid() {
    if (!crc16_check_buffer((uint8_t *) (&badge_conf), sizeof(qc15conf)-2))
        return 0;

    if (badge_conf.badge_id > QC15_BADGES_IN_SYSTEM)
        return 0;

    if (badge_conf.badge_name[0] == 0xFF)
        return 0;

    return 1;
}

/// Validate, load, and/or generate this badge's configuration as appropriate.
void init_config() {
    // Check the stored FRAM config:
    if (config_is_valid()) return;

    // If that's bad, try the backup:
    memcpy(&badge_conf, &backup_conf, sizeof(qc15conf));
    if (config_is_valid()) return;

    // If we're still here, none of the three config sources were valid, and
    //  we must generate a new one.
    generate_config();

    srand(badge_conf.badge_id);
}

extern uint8_t bootstrap_completed;

/// The main initialization and loop function.
void main (void)
{
    init();

    uint8_t initial_buttons = P9IN;

    __bis_SR_register(GIE);

    if (!(initial_buttons & BIT4)) {
        bootstrap_completed = 0;
    }

    // Do verbose boot for the diagnostic code:
    bootstrap(initial_buttons & BIT4);
    // Allow radio messages:

    ht16d_all_one_color(0, 0, 0);

    uint8_t last_tx = 0;
    uint8_t last_rx = 0;
    char out[25] = "";
    if (1 || !bootstrap_completed) {
        out_of_bootstrap = 1;

        lcd111_set_text(LCD_TOP, "RF test:  Wht:TX, Grn:RX");
        lcd111_set_text(LCD_BTM, "DOWN: OK, LEFT: FAIL");

        while (1) {
            handle_global_signals();

            if (last_tx != tx_cnt || last_rx != rx_cnt) {
                last_tx = tx_cnt;
                last_rx = rx_cnt;
                sprintf(out, "TX CNT:%d  RX CNT:%d", tx_cnt, rx_cnt);
                lcd111_set_text(LCD_TOP, out);
            }

            if (s_down) {
                ht16d_all_one_color(0, 0, 0);
                break;
            }

            if (s_right) {
                while (!ipc_tx_byte(IPC_MSG_REBOOT));
            }

            if (s_left) {
                ht16d_all_one_color(0, 0, 0);
                bootstrap_completed = 0;
                PMMCTL0 |= PMMSWPOR; // Software reboot.
                break;
            }

            cleanup_global_signals();
        }

        bootstrap_completed = 1;
        out_of_bootstrap = 0;
    }

    // Allow flash loading:
    // (loops forever)
    flash_bootstrap();
}

/// The time loop ISR, at vector `0xFFDE` (`Timer1_A3 CC0`).
#pragma vector=TIMER1_A0_VECTOR
__interrupt
void TIMER_ISR() {
    // All we have here is CCIFG0
    f_time_loop = 1;
    LPM_EXIT;
}
