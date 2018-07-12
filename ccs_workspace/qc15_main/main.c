#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

#define POST_MCU 0
#define POST_LCD 1
#define POST_LED 2
#define POST_NOR 3
#define POST_IPC 4
#define POST_UPNEXT 5
#define POST_UP 6
#define POST_DOWN 7
#define POST_LEFT 8
#define POST_RIGHT 9
#define POST_SW1 10
#define POST_SW2 11
#define POST_OK 12

void bootstrap(uint8_t fastboot) {
    uint8_t rx_from_radio[IPC_MSG_LEN_MAX] = {0};
    uint8_t bootstrap_status = 0;
    uint16_t time_32nd_secs = 0;

    // 0->1   MCU POST
    // 1->2   LCD POST
    // 2->3   LED POST
    // 3->4   NOR POST
    // 4->5   IPC POST w/ RADIOMCU POST STATUS
    // 5->6   UP
    // 6->7   DOWN
    // 7->8   LEFT
    // 8->9   RIGHT
    // 9->10  SWITCH TOGGLE
    // 10->11 SWITCH TOGGLE
    // 11->12 OK w/ any feedback

    // Tell the radio module to reboot.
    ipc_tx_byte(IPC_MSG_REBOOT);

    if (bootstrap_status == POST_MCU) {
        bootstrap_status++;
    }

    if (bootstrap_status == POST_LCD) {
        bootstrap_status++;
        if (!fastboot) {
            lcd111_text(1, "QC15 BOOTSTRAP>");
            lcd111_text(0, "LCD POST: OK");
            delay_millis(200);
        }
    }

    if (bootstrap_status == POST_LED) {
        if (ht16d_post()) {
            bootstrap_status++;
            if (!fastboot) {
                lcd111_text(0, "LED driver POST: OK");
                led_all_one_color(100, 100, 100);
                delay_millis(200);
            }
        } else {
            // POST appears to have failed.
            lcd111_text(1, "QC15 BOOTSTRAP> FAIL");
            lcd111_text(0, "LED driver POST: FAIL");
            delay_millis(2000);
            bootstrap_status++;
        }
    }

    if (bootstrap_status == POST_NOR) {
        if (s25flash_post()) {
            bootstrap_status++;
            if (!fastboot) {
                lcd111_text(0, "SPI NOR flash POST: OK");
                delay_millis(200);
            }
        } else {
            // TODO: This is FATAL, probably.
            //  We should do an animation or something so the badge does
            //  _something_.
            lcd111_text(1, "QC15 BOOTSTRAP> FAIL");
            lcd111_text(0, "SPI NOR flash POST FAIL!");
            led_all_one_color(200, 0, 0);
            delay_millis(2000);
            bootstrap_status++;
        }
    }

    while (1) {
        if (f_time_loop) {
            f_time_loop = 0;
            time_32nd_secs++;
            poll_buttons();
        }

        // Received an IPC message
        if (f_ipc_rx) {
            f_ipc_rx = 0;
            // If it's valid...
            if (ipc_get_rx(rx_from_radio)) {
                if ((rx_from_radio[0] & 0xF0) == IPC_MSG_POST) {
                    // Give the correct response.
                    ipc_tx_byte(IPC_MSG_POST); // TODO: this will change
                    if (bootstrap_status == POST_IPC) {
                        // got a POST message.
                        if (rx_from_radio[0] & 0x0F) {
                            lcd111_text(1, "QC15 BOOTSTRAP> FAIL");
                            led_all_one_color(200, 50, 0);
                            // it indicates a FAILURE on the radio side
                            if (rx_from_radio[0] & BIT0) {
                                // MCU fail
                                lcd111_text(0, "RADIOMCU: MCU FAIL");
                                delay_millis(2000);
                            }
                            if (rx_from_radio[0] & BIT1) {
                                // XT1 fail
                                lcd111_text(0, "RADIOMCU: XT1 FAIL");
                                delay_millis(2000);
                            }
                            if (rx_from_radio[0] & BIT2) {
                                // RFM75 fail
                                lcd111_text(0, "RADIOMCU: RFM75 FAIL");
                                delay_millis(2000);
                            }
                            bootstrap_status++;
                        } else {
                            // all good.
                            bootstrap_status++;
                            if (!fastboot) {
                                lcd111_text(0, "IPC POST: OK");
                                delay_millis(200);
                            }
                        }
                    }
                }

                if (bootstrap_status == POST_SW1 &&
                        (rx_from_radio[0] & 0xF0) == IPC_MSG_SWITCH) {
                    lcd111_text(0, "POST: Toggle switch back");
                    bootstrap_status++;
                } else if (bootstrap_status == POST_SW2 &&
                        (rx_from_radio[0] & 0xF0) == IPC_MSG_SWITCH) {
                    lcd111_text(0, "POST: Buttons OK");
                    delay_millis(1000);
                    lcd111_text(0, "Click UP to leave POST");
                    bootstrap_status++;
                    break;
                }

            } else {
                // CRC fail. It will resend.
            }
        }

        if (time_32nd_secs == 128 && bootstrap_status == POST_IPC) {
            time_32nd_secs = 0;
            lcd111_text(1, "QC15 BOOTSTRAP> FAIL");
            lcd111_text(0, "IPC POST> general fail");
            led_all_one_color(200, 0, 0);
            delay_millis(2000);
            bootstrap_status++;
        }

        if (bootstrap_status == POST_UPNEXT) {
            if (fastboot) {
                bootstrap_status = POST_OK;
                break;
            }
            lcd111_text(0, "POST: Click UP.");
            bootstrap_status++;
        }

        if (s_buttons) {
            if (s_buttons & BIT0) { // DOWN
                if (s_buttons & BIT4) {
                    if (bootstrap_status == POST_DOWN) {
                        lcd111_text(0, "POST: Click LEFT.");
                        bootstrap_status++;
                    }
                } else {
                    // press
                }
            }

            if (s_buttons & BIT1) {
                if (s_buttons & BIT5) { // RIGHT
                    if (bootstrap_status == POST_RIGHT) {
                        lcd111_text(0, "POST: Toggle switch.");
                        bootstrap_status++;
                    }
                } else {
                    // press
                }
            }
            if (s_buttons & BIT2) {
                if (s_buttons & BIT6) {
                    if (bootstrap_status == POST_LEFT) {
                        lcd111_text(0, "POST: Click RIGHT.");
                        bootstrap_status++;
                    }
                } else {
                    // press
                }
            }
            if (s_buttons & BIT3) {
                if (s_buttons & BIT7) {
                    if (bootstrap_status == POST_UP) {
                        lcd111_text(0, "POST: Click DOWN.");
                        bootstrap_status++;
                    }
                } else {
                    // press
                }
            }
            s_buttons = 0;
        }
    }

}

void main (void)
{
    init();

    __bis_SR_register(GIE);
    bootstrap(0);

    uint8_t rx_from_radio[IPC_MSG_LEN_MAX] = {0};

    lcd111_text(0, "Queercon 15");
    lcd111_text(1, "UBER BADGE");



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
            if (ipc_get_rx(rx_from_radio)) {
                __no_operation();
            }
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
