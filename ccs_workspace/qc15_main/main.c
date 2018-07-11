#include <stdint.h>
#include <stdlib.h>

#include <msp430fr5972.h>
#include <driverlib.h>
#include <s25flash.h>

//#include "qc15.h"

#include "util.h"

#include "lcd111.h"
#include "ht16d35b.h"
#include "s25flash.h"
#include "ipc.h"
#include "leds.h"

volatile uint8_t f_time_loop = 0;
uint8_t s_buttons = 0;

void init_clocks() {

    // CLOCK SOURCES
    // =============

    // Fixed sources:
    //      VLO      10k Very low power low-frequency oscillator
    //      MODOSC   5M  for MODCLK
    //      LFMODCLK (MODCLK/128, 39 kHz)

    // Configurable sources:
    //      DCO  (Digitally-controlled oscillator) (8 MHz)

    //      LFXT (Low frequency external crystal) - unused
    //      HFXT (High frequency external crystal) - unused

    // SYSTEM CLOCKS
    // =============

    // MCLK (8 MHz)
    //  Defaults to DCOCLK /8
    //  Available sources are HFXT, DCO, LFXT, VLO, or external digital clock.
    //   If it's above 8 MHz, we need to configure FRAM wait-states.
    //   Set to 8 MHz (DCO /2)
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1); // 8 M

    // SMCLK (1 MHz)
    //  Defaults to DCOCLK /8
    //  Same sources available as MCLK.
    //      NB: This is different from the SMCLK behavior of the FR2xxx series,
    //          which can only source SMCLK from a divided MCLK.
    //  We'll use DCO /16
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_8); // 1 M

    // MODCLK (5 MHz)
    //  This comes from MODOSC. It's fixed.

    // ACLK
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

const rgbcolor_t rainbow_colors[] = {
        {255, 0, 0}, // Red
        {255, 24, 0x00}, // Orange
        {128, 40, 0x00}, // Yellow
        {0, 64, 0}, // Green
        {0, 0, 196}, // Blue
        {128, 0, 128}, // Purple
};

const led_ring_animation_t anim_rainbow = {
        &rainbow_colors[0],
        6,
        5,
        "Rainbow"
};

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
            // Upper nibble is its current value:
            //  (so if it's 1, it was just RELEASED,
            //    & if it's 0, it was just PRESSED.)
            s_buttons |= (button_read & (BIT4<<i));
        }
    }
    button_read_prev = button_read;
} // poll_buttons


void main (void)
{
    init();

    lcd111_text(0, "Queercon 15");
    lcd111_text(1, "UBER BADGE");

    uint8_t rx_from_radio[IPC_MSG_LEN_MAX] = {0};

    __bis_SR_register(GIE);

    led_set_anim(
        (led_ring_animation_t *) &anim_rainbow,
        LED_ANIM_TYPE_FALL,
        5,
        1
    );

    while (1) {
        if (f_time_loop) {
            f_time_loop = 0;
            led_timestep();
            poll_buttons();
        }

        if (f_ipc_rx) {
            f_ipc_rx = 0;
            ipc_get_rx(rx_from_radio);
        }

        if (s_led_anim_done) {
            s_led_anim_done = 0;
            led_set_anim((led_ring_animation_t *) &anim_rainbow,
                         LED_ANIM_TYPE_SPIN, 2, 18);
        }

        if (s_buttons) {
            if (s_buttons & BIT0) {
                if (s_buttons & BIT4) {
                    // release
                } else {
                    // press
                }
            }

            if (s_buttons & BIT1) {
                if (s_buttons & BIT5) {
                    // release
                } else {
                    // press
                }
            }
            if (s_buttons & BIT2) {
                if (s_buttons & BIT6) {
                    // release
                } else {
                    // press
                }
            }
            if (s_buttons & BIT3) {
                if (s_buttons & BIT7) {
                    // release
                } else {
                    // press
                }
            }
            s_buttons = 0;
        }

        // Go to sleep.
        LPM0;
    }
}

// 0xFFDE Timer1_A3 CC0
#pragma vector=TIMER1_A0_VECTOR
__interrupt
void TIMER_ISR() {
    // All we have here is CCIFG0
    f_time_loop = 1;
    LPM0_EXIT;
}
