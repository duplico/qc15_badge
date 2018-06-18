#include <stdint.h>

#include <msp430fr5972.h>
#include <driverlib.h>
#include <s25flash.h>

// TODO: Create:
//#include "qc15.h"
// TODO: Create a conf.h
#include "util.h"

#include "lcd111.h"
#include "ht16d35b.h"
#include "s25flash.h"
#include "ipc.h"

void init_clocks() {
    /*
     * On power-on, the following defaults apply in the clock system:
     *  - LFXT selected as oscillator source for LFXTCLK (not populated)
     *  - ACLK: undivided, LFXTCLK
     *  - MCLK: DCOCLK (/8)
     *  - SMCLK:DCOCLK (/8)
     *  - LFXIN/LFXOUT are GPIO and LFXT is disabled
     */

    /*
     * Available sources:
     *  - LFXTCLK (not available)
     *  - VLOCLK (very low power (100 nA), 10-kHz)
     *  - DCOCLK (selectable DCO)
     *  - MODCLK (low-power (25 uA), 5 MHz)
     *  - LFMODCLK (MODCLK/128, 39 kHz)
     *  - HFXTCLK (not available)
     */

    CS_setDCOFreq(1, 4); // Set DCO to 16 MHz

    CS_initClockSignal(CS_ACLK, CS_LFMODOSC_SELECT, CS_CLOCK_DIVIDER_1); // 39k
    // NB: If MCLK is over 8 MHz we have to mess around with FRAM wait states.
    //     I'd rather not do that.
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_2); // 8 M
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_8); // 1 M
}

void init_io() {
    // The magic make-it-work command.
    //  Otherwise everything is stuck in high-impedance forever.
    PMM_unlockLPM5();

    lcd111_init_io();
    ht16d_init_io();
    s25flash_init_io();
    ipc_init_io();

    // Screw post inputs with pull-ups
    P7DIR &= ~(GPIO_PIN2+GPIO_PIN3+GPIO_PIN4); // inputs
    P7REN |= GPIO_PIN2+GPIO_PIN3+GPIO_PIN4; // resistor enable
    P7OUT |= GPIO_PIN2+GPIO_PIN3+GPIO_PIN4; // pull-up resistor

    // Buttons: 9.4, 9.5, 9.6, 9.7
    P9DIR &= ~(BIT4+BIT5+BIT6+BIT7); // inputs
    P9REN |= (BIT4+BIT5+BIT6+BIT7); // 0xf0
    P9OUT |= 0xf0; // pull up, please.
}

void init() {
    // TODO: Enable WDT
    WDT_A_hold(WDT_A_BASE);

    init_clocks();

    init_io();

    lcd111_init();
    ht16d_init();
    s25flash_init();
    ipc_init();
}

void main (void)
{
    init();

    lcd111_text(0, "Test 0");
    lcd111_text(1, "Test 1");
    led_send_gray();

    while (1);
}
