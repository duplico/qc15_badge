/*
 * main.c
 *
 *  Created on: Jun 15, 2018
 *      Author: @duplico
 */


/*
 * P1.0 radio CS
 * P1.1 radio CLK
 * P1.2 radio SIMO
 * P1.3 radio SOMI
 * P1.4 IPC TX (A0TX)
 * P1.5 IPC RX (A0RX)
 * P1.6 radio EN
 * P1.7 radio IRQ
 * P2.0 XTAL
 * P2.1 XTAL
 * P2.2 switch (right is high)
 *   |----/\/\/\/\/-----VCC
 */

#include <stdint.h>

#include "driverlib.h"
#include <msp430fr2422.h>

#include "radio.h"
#include "ipc.h"
#include "util.h"

#include "qc15.h"

volatile uint8_t f_time_loop = 0;
uint8_t s_switch = 0;
uint8_t s_radio_interval = 0;

volatile uint64_t qc_clock = 0;
uint8_t sw_state = 0;

qc15status badge_status = {0};

void bootstrap();

/// Initialize IO configuration for the radio MCU.
/**
 * Specifically, this module unlocks the IO pins from high-impedance mode,
 * and then configures the peripheral and GPIO settings for:
 * * IPC TX/RX
 * * Crystal input/output
 * * Power switch
 *
 * It leaves the remainder of the peripherals to be configured by their
 * dedicated initialization functions.
 *
 * It also reads the initial value of the switch into `sw_state`, so that
 * we don't have a signal fire if the switch is in a different position
 * than the 0-initialized one when we turn on the device.
 */
void init_io() {
    // The magic FRAM make-it-work command:
    PMM_unlockLPM5();

    // Radio IO is all set inside the radio driver. For now.

    // Port 1:
    // -------
    // * P1.0 RFM CSN (GPIO)    (configured in radio driver)
    // * P1.1 RFM CLK (Primary) (configured in radio driver)
    // * P1.2 RFM MO  (Primary) (configured in radio driver)
    // * P1.3 RFM MI  (Primary) (configured in radio driver)
    // * P1.4 IPC TX  (Primary)
    // * P1.5 IPC RX  (Primary)
    // * P1.6 RFM CE  (GPIO)    (configured in radio driver)
    // * P1.7 RFM IRQ (GPIO in) (configured in radio driver)

    // IPC TX/RX
    P1SEL0 |= BIT4+BIT5;
    P1SEL1 &= ~(BIT4+BIT5); // unneeded but whatever.

    // Port 2:
    // -------
    // * P2.0 LFXT (Primary)
    // * P2.1 LFXT (Primary)
    // * P2.2 PWSW (GPIO in w/ pull-up) (right/down is HIGH)
    //
    //   There are no other usable GPIO pins on the device.

    P2SEL1 = 0b011; // MSB
    P2SEL0 = 0b000; // LSB
    P2DIR &= ~BIT2; // Switch pin set to input.
    P2REN |= BIT2;  // Switch resistor enable
    P2OUT |= BIT2;  // Switch resistor pull UP direction

    sw_state = P2IN & BIT2; // Read the switch's initial value.
}

/// Initialize all the clock sources for the radio MCU.
void init_clocks() {
    // CLOCK SOURCES
    // =============

    // Fixed sources:
    //      REFO     32k Integrated 32 kHz RC oscillator
    //      VLO      10k Very low power low-frequency oscillator
    //      MODOSC   5M  for MODCLK

    // Configurable sources:
    // LFXT (Low frequency external crystal)
    CS_turnOnXT1LF(
            CS_XT1_DRIVE_0
    );

    CS_initClockSignal(
            CS_ACLK,
            CS_XT1CLK_SELECT,
            CS_CLOCK_DIVIDER_1
    );

    //clear all OSC fault flag
    CS_clearAllOscFlagsWithTimeout(1000);

    // DCO  (Digitally-controlled oscillator)
    //  Let's bring this up to 8 MHz or so.

    __bis_SR_register(SCG0);                // disable FLL
    CSCTL3 |= SELREF__XT1CLK;               // Set XT1CLK as FLL reference source
    CSCTL0 = 0;                             // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);                 // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_3;                    // Set DCO = 8MHz
    CSCTL2 = FLLD_0 + 243;                  // DCODIV = /1
//    CSCTL2 = FLLD_3 + 243;                  // DCODIV = /8
    __delay_cycles(3);
    __bic_SR_register(SCG0);                // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked

    CS_clearAllOscFlagsWithTimeout(1000);

    // SYSTEM CLOCKS
    // =============

    // MCLK (1 MHz)
    //  All sources but MODOSC are available at up to /128
    //  Set to DCO/8 = 1 MHz
    // SMCLK (1 MHz)
    //  Derived from MCLK with divider up to /8
    //  Set to MCLK/1, which we'll keep.

    CSCTL5 |= DIVM_3 | DIVS_0;

    // MODCLK (5 MHz)
    //  This comes from MODOSC

    // ACLK
    //  Initializes to REFO, which is ~ 32k.
    //  This is OK, but we'd rather have it connected to our watch crystal,
    //   which will give us a more precise 32k signal.

    CS_initClockSignal(
            CS_ACLK,
            CS_XT1CLK_SELECT,
            CS_CLOCK_DIVIDER_1
    );
//    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;  // Set ACLK = XT1CLK = 32768Hz

}

/// Set up the real time clock to give us a tick every 1/32 sec.
/**
 * This is a much less sophisticated RTC than we've used in the past on the
 * higher-end F and FR series MSP430s, but it's very simple to configure.
 * This configures it as a simple timer, firing an interrupt every 1/32
 * second. In this configuration, the `qc_clock` ticks have the following
 * values:
 *
 * * Second - 32 ticks
 * * Minute - 1,920 ticks
 * * Hour -  115,200 ticks
 * * Day - 2,764,800 ticks
 * * Conference - 11,059,200 ticks (approx 4 days)
 *
 * This means that 24 bits gives us about the right length to hold the
 * ticks since the beginning of the conference, and then we can max out the
 * counter at 2^24.
 */
void rtc_init() {
    // This is a much less sophisticated RTC than we've used in the past, on
    //  the higher end MSP430F and MSP430FR processors, but it's simple to
    //  configure, leaving it to software to do any of the "alarms" or
    //  interpretation of its value as actual dates and times. That's fine,
    //  because binary coded decimals hurt my brain box.

    // The RTC counter will tick for every cycle of its clock source (XT1,
    //  our external 32k crystal.)
    // We want an interrupt to fire every 1/32 second, so we're going to set
    //  the modulo register to be 0x0400 (1024, which is 32k/32).
    RTCMOD = 0x0400;
    // Now let's setup the control register. We're going to use ACLK as our
    //  clock source, so that we can use its fallback methods for clock
    //  sourcing to REFO, rather than having to mess around with that here.
    //  This means that even if XT1 fails we should have a reasonably good
    //  approximation of a 32k clock signal.

    // In order to use SMCLK or ACLK as an input, we need to do two things.
    //  We select that clock source for the RTC by setting RTCCTL.RTCSS=0b01,
    //  but first we need to configure register SYSCFG2.RTCCLK=RTC_ACLK (0b1).
//    SYSCFG2 |= RTCCKSEL__RTC_ACLK;

    // RTCSR_1 means "clear the counter and reload from RTCMOD."
//    RTCCTL = RTCSS_1  | RTCSR_1;
    RTCCTL = RTCSS__XT1CLK | RTCSR_1;
//    RTCIV; // Read the vector to clear the interrupt.

    RTCCTL |= RTCIE; // Enable the interrupt.
}

void poll_switch() {
    // right is HIGH ("off")
    static uint8_t sw_read_prev = 0;
    static uint8_t sw_read = 0;

    sw_read = P2IN & BIT2;
    if (sw_read == sw_read_prev && sw_read != sw_state) {
        // We're (a) debounced, and (b) detecting a change:
        sw_state = sw_read;
        // raise a signal:
        s_switch = 1;
    }
    sw_read_prev = sw_read;
}

void handle_ipc_rx(uint8_t *rx_from_radio) {
    switch(rx_from_radio[0]) {
    case IPC_MSG_REBOOT:
        PMMCTL0 |= PMMSWPOR; // Software reboot.
        break; // though this hardly seems necessary.
    case IPC_MSG_STATS_ANS:
        // A response to our request for a stats update:
        memcpy(&badge_status, &rx_from_radio[1], sizeof(qc15status));
        break;
    default:
        break;
    }
}

void main (void)
{
    uint8_t rx_from_main[IPC_MSG_LEN_MAX] = {0};

    WDT_A_hold(WDT_A_BASE);

    init_io();
    init_clocks();
    ipc_init();
    rtc_init();
    radio_init();


    __bis_SR_register(GIE);

    bootstrap();

    while (1) {
        if (f_rfm75_interrupt) {
            rfm75_deferred_interrupt();
        }

        if (f_time_loop) {
            f_time_loop = 0;
            poll_switch();
            if (qc_clock % 512) {
                // Every 16 seconds,
                s_radio_interval = 1;
            }
        }

        if (s_radio_interval) {
            // Calling radio_interval() has _lots_ of side effects, and also
            //  does a radio TX. We need the TX to happen for this interval
            //  to really be valid, so if we can't do the TX we should just
            //  defer the radio interval until the next time a transmit is
            //  allowed.
            if (rfm75_tx_avail()) {
                s_radio_interval = 0;
                radio_interval();
            }
        }

        if (f_ipc_rx) {
            f_ipc_rx = 0;
            if (ipc_get_rx(rx_from_main)) {
                handle_ipc_rx(rx_from_main);
            }
        }

        if (s_switch) {
            // The switch has been toggled. So we need to send a message to
            //  that effect. This is a fairly important message, so we'll
            //  keep trying to send it every time we get here, until it
            //  succeeds. But we're not going to wait for an ACK.
            // Because the switch is "active low" (that is, LEFT
            //  is "ON" and corresponds to LOW), we're going to take this
            //  opportunity to evaluate sw_state and reverse it.
            if (ipc_tx_byte(IPC_MSG_SWITCH | (sw_state ? 0 : 1))) {
                s_switch = 0;
            }
        }

        LPM;
    }
}

#pragma vector=RTC_VECTOR
__interrupt
void RTC_ISR() {
    if (RTCIV == RTCIV__RTCIFG) {
        f_time_loop = 1;
        qc_clock++;
        LPM_EXIT;
    }
}
