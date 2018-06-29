/*
 * rfm75.c
 *
 * Queercon 15 radio driver for the HopeRF RFM75.
 *
 * (c) 2018 George Louthan
 * 3-clause BSD license; see license.md.
 */

#include <string.h>
#include <stdint.h>

#include <driverlib.h>

#include "rfm75.h"

// Handy generic pin twiddling:
#define CSN_LOW_START RFM75_CSN_OUT &= ~RFM75_CSN_PIN
#define CSN_HIGH_END  RFM75_CSN_OUT |= RFM75_CSN_PIN

#define CE_ACTIVATE RFM75_CE_OUT |= RFM75_CE_PIN
#define CE_DEACTIVATE RFM75_CE_OUT &= ~RFM75_CE_PIN

// Local vars and buffers:
uint16_t rfm75_unicast_addr = 0;
const uint16_t rfm75_broadcast_addr = 0xffff;
#define UNICAST_LSB 0
#define BROADCAST_LSB 0xEE

uint8_t rx_addr_p0[3] = {UNICAST_LSB, 100, 0};
uint8_t rx_addr_p1[3] = {BROADCAST_LSB, 0xff, 0xff};
uint8_t payload_in[RFM75_PAYLOAD_SIZE] = {0};
uint8_t payload_out[RFM75_PAYLOAD_SIZE] = {0};

uint8_t rfm75_state = RFM75_BOOT;

volatile uint8_t f_rfm75_interrupt = 0;

///////////////////////////////
// Bank initialization values:

#define BANK0_INITS 17
const uint8_t bank0_init_data[BANK0_INITS][2] = {
        { CONFIG, 0xff }, //
        { 0x01, BIT0+BIT1 }, // Auto-ack for pipe0 (unicast)
        { 0x02, BIT0+BIT1 }, //Enable RX pipe 0 and 1
        { 0x03, 0b00000001 }, //RX/TX address field width 3byte
        { 0x04, 0b00000100 }, //auto-RT // TODO
        { 0x05, 0x53 }, //channel: 2400 + LS 7 bits of this field = channel (2.483)
        { 0x06, 0b00000111 }, //air data rate-1M,out power max, setup LNA gain high.
        { 0x07, 0b01110000 }, // Clear interrupt flags
        // 0x0a - RX_ADDR_P0 - 3 bytes
        // 0x0b - RX_ADDR_P1 - 3 bytes
        // 0x10 - TX_ADDR - 5 bytes
        { 0x11, RFM75_PAYLOAD_SIZE }, //Number of bytes in RX payload in data pipe0
        { 0x12, RFM75_PAYLOAD_SIZE }, //Number of bytes in RX payload in data pipe1
        { 0x13, 0 }, //Number of bytes in RX payload in data pipe2 - disable
        { 0x14, 0 }, //Number of bytes in RX payload in data pipe3 - disable
        { 0x15, 0 }, //Number of bytes in RX payload in data pipe4 - disable
        { 0x16, 0 }, //Number of bytes in RX payload in data pipe5 - disable
        { 0x17, 0 },
        { 0x1c, 0x00 }, // No dynamic packet length
        { 0x1d, 0b00000001 } // 00000 | DPL | ACK_PAYLOAD | DYN_ACK
};

uint8_t rfm75spi_recv_sync(uint8_t data) {
    while (!(RFM75_UCxIFG & UCTXIFG));
    RFM75_UCxTXBUF = data;
    while (!(RFM75_UCxIFG & UCRXIFG));
    return RFM75_UCxRXBUF;
}

void rfm75spi_send_sync(uint8_t data) {
    rfm75spi_recv_sync(data);
}

uint8_t rfm75_get_status() {
    uint8_t recv;
    CSN_LOW_START;
    recv = rfm75spi_recv_sync(NOP_NOP);
    CSN_HIGH_END;
    return recv;
}

uint8_t send_rfm75_cmd(uint8_t cmd, uint8_t data) {
    uint8_t ret;
    CSN_LOW_START;
    rfm75spi_send_sync(cmd);
    ret = rfm75spi_recv_sync(data);
    CSN_HIGH_END;
    return ret;
}

void send_rfm75_cmd_buf(uint8_t cmd, uint8_t *data, uint8_t data_len) {
    CSN_LOW_START;
    rfm75spi_send_sync(cmd);
    for (uint8_t i=1; i<=data_len; i++) {
        rfm75spi_send_sync(data[data_len-i]);
    }
    CSN_HIGH_END;
}

void read_rfm75_cmd_buf(uint8_t cmd, uint8_t *data, uint8_t data_len) {
    CSN_LOW_START;
    rfm75spi_send_sync(cmd);
    for (uint8_t i=1; i<=data_len; i++) {
        data[data_len-i] = rfm75spi_recv_sync(0xab);
    }
    CSN_HIGH_END;
}

uint8_t rfm75_read_reg(uint8_t cmd) {
    cmd &= 0b00011111;
    CSN_LOW_START;
    rfm75spi_send_sync(cmd);
    uint8_t recv = rfm75spi_recv_sync(0xff);
    CSN_HIGH_END;
    return recv;
}

void rfm75_write_reg(uint8_t reg, uint8_t data) {
    reg &= 0b00011111;
    send_rfm75_cmd(WRITE_REG | reg, data);
}

void rfm75_write_reg_buf(uint8_t reg, uint8_t *data, uint8_t data_len) {
    reg &= 0b00011111;
    send_rfm75_cmd_buf(WRITE_REG | reg, data, data_len);
}

void rfm75_select_bank(uint8_t bank) {
    volatile uint8_t currbank = rfm75_get_status() & 0x80; // Get MSB, which is active bank.
    if ((currbank && (bank==0)) || ((currbank==0) && bank)) {
        send_rfm75_cmd(ACTIVATE_CMD, 0x53);
    }
}

void set_unicast_addr(uint16_t addr) {
    rx_addr_p0[0] = UNICAST_LSB;
    rx_addr_p0[1] = addr & 0xff;
    rx_addr_p0[2] = (addr & 0xff00) >> 8; // MSB
    rfm75_write_reg_buf(RX_ADDR_P0, rx_addr_p0, 3);
}


uint8_t rfm75_post() {
    volatile uint8_t bank_one = rfm75_get_status() & 0x80; // Get MSB, which is active bank.
    send_rfm75_cmd(ACTIVATE_CMD, 0x53);
    volatile uint8_t bank_two = rfm75_get_status() & 0x80; // Get MSB, which is active bank.

    rfm75_select_bank(0); // Go back to the normal bank.

    // The following is an alternative POST that was used at some point.
    // TODO: Decide what to do with this:
    //
    //    volatile uint8_t temp = 0;
    //    volatile uint8_t test = 0;
    //    for(uint8_t i=0;i<BANK0_INITS;i++) {
    //        temp = rfm75_read_byte(bank0_init_data[i][0]);
    //        test = temp == bank0_init_data[i][1];
    //    }

    if (bank_one == bank_two) {
        return 0;
    }
    return 1;
}

void rfm75_enter_prx() {
    rfm75_state = RFM75_RX_INIT;
    CE_DEACTIVATE;
    // Power up & enter PRX (Primary RX)
    set_unicast_addr(rfm75_unicast_addr);
    rfm75_select_bank(0); // TODO: We don't need all these.
    rfm75_write_reg(CONFIG, CONFIG_MASK_TX_DS +
                    CONFIG_MASK_MAX_RT + CONFIG_EN_CRC +
                    CONFIG_CRCO_2BYTE + CONFIG_PWR_UP +
                    CONFIG_PRIM_RX);


    // Clear interrupts: STATUS=BIT4|BIT5|BIT6
    rfm75_write_reg(STATUS, BIT4|BIT5|BIT6);

    // Enter RX mode.
    CE_ACTIVATE;

    rfm75_state = RFM75_RX_LISTEN;
}

void rfm75_tx(uint16_t addr) {
    rfm75_state = RFM75_TX_INIT;
    uint8_t wr_cmd = WR_TX_PLOAD_NOACK;

    CE_DEACTIVATE;
    rfm75_select_bank(0);

    rfm75_write_reg(CONFIG, CONFIG_MASK_RX_DR +
                            CONFIG_EN_CRC + CONFIG_CRCO_2BYTE +
                            CONFIG_PWR_UP + CONFIG_PRIM_TX);
//                    0b01011110);

    // Setup our destination address:

    uint8_t tx_addr[3] = {0};

    if (addr == rfm75_broadcast_addr) {
        // broadcast!
        tx_addr[0] = BROADCAST_LSB;
    } else {
        // unicast!
        // Since we're going to listen for ACKs, we need to change our P0 ADDR
        //  to be the same as the destination address.
        set_unicast_addr(addr);
        tx_addr[0] = UNICAST_LSB;
        wr_cmd = WR_TX_PLOAD; // request an ACK.
    }

    tx_addr[1] = addr & 0xff;
    tx_addr[2] = (addr & 0xff00) >> 8; // MSB

    rfm75_write_reg_buf(TX_ADDR, tx_addr, 3);

    // Clear interrupts: STATUS=BIT4|BIT5|BIT6
    rfm75_write_reg(STATUS, BIT4|BIT5|BIT6);

    rfm75_state = RFM75_TX_FIFO;
    // Write the payload:
    send_rfm75_cmd_buf(wr_cmd, payload_out, RFM75_PAYLOAD_SIZE);
    rfm75_state = RFM75_TX_SEND;
    CE_ACTIVATE;
    // Now we wait for an IRQ to let us know it's sent.
}

void rfm75_io_init() {
    // CSN
    RFM75_CSN_DIR |= RFM75_CSN_PIN;
    CSN_HIGH_END; // initialize deselected.
    // CE (1.6):
    RFM75_CE_DIR |= RFM75_CE_PIN;
    CE_DEACTIVATE; // initialize deactivated.
    // IRQ (1.7):
    RFM75_IRQ_DIR &= ~RFM75_IRQ_PIN;
    RFM75_IRQ_REN &= ~RFM75_IRQ_PIN;
    RFM75_IRQ_SEL0 &= ~RFM75_IRQ_PIN;
    RFM75_IRQ_SEL1 &= ~RFM75_IRQ_PIN;
    // Pins for the USCI:
    GPIO_setAsPeripheralModuleFunctionOutputPin(RFM75_USCI_PORT,
                                                RFM75_USCI_PINS,
                                                GPIO_PRIMARY_MODULE_FUNCTION);
    // Setup USCI
    RFM75_UCxCTLW0 |= UCSWRST;
    RFM75_UCxBRW = (uint16_t)(CS_getSMCLK() / 1000000); // set clock scaler
    // clear control word:
    RFM75_UCxCTLW0 &= ~(UCCKPH + UCCKPL + UC7BIT + UCMSB +
            UCMST + UCMODE_3 + UCSYNC + UCSSEL_3);
    // set control word:
    RFM75_UCxCTLW0 |=    UCSSEL__SMCLK + // use SMCLK
            UCMSB + //MSB first
            UCCKPH + // mode 01 or whatever
            UCMODE_0 + // 3-pin
            UCMST + // master mode
            UCSYNC; // synchronous mode.

    // and enable!
    RFM75_UCxCTLW0 &= ~UCSWRST;

}

void rfm75_init(uint16_t unicast_address)
{
    rfm75_io_init();

    // We're going totally synchronous on this; no interrupts at all.
    // We'll wait on the interrupt enables though, until after we've set up
    //  all of our register banks with the initial configuration

    // Let's start with bank 0:
    rfm75_select_bank(0);

    // Because we want the option of using dynamic ACKs, and possibly dynamic
    //  packet lengths as well, we need to determine whether this radio has
    //  had its FEATURE register ACTIVATED (by sending the ACTIVATE command
    //  followed by 0x73.)
    // But since a restart of our code does not necessarily mean that the radio
    //  has been restarted by a power cycle, and because the ACTIVATE command
    //  is a toggle,
    uint8_t test_feature_reg = 0;
    test_feature_reg = rfm75_read_reg(FEATURE);
    rfm75_write_reg(FEATURE, test_feature_reg ^ 0b00000111); // flip all the bits.
    if (rfm75_read_reg(FEATURE) == test_feature_reg) {
        // In spite of trying to flip these, bits, they stayed the same.
        // Therefore, it needs to be ACTIVATED.
        send_rfm75_cmd(ACTIVATE_CMD, 0x73);
    } else {
        // else we're already ACTIVATED.
        // no need to run any command.
        // (and the FEATURE register will be configured in the master bank0
        //  config for loop, so there's no need to try to undo our bit flips
        //  here.)
    }

    for(uint8_t i=0;i<BANK0_INITS;i++)
        rfm75_write_reg(bank0_init_data[i][0], bank0_init_data[i][1]);

    // Setup addresses:
    rfm75_unicast_addr = unicast_address;
    rfm75_write_reg_buf(RX_ADDR_P1, rx_addr_p1, 3);

    // OK, that's bank 0 done. Next is bank 1.

    rfm75_select_bank(1);

    // Some of these go MOST SIGNIFICANT BYTE FIRST: (so we start with 0xE2.)
    //  (we show them here LEAST SIGNIFICANT BYTE FIRST because we
    //   reverse everything we send.)
    // Basically, these are just stupid magic numbers that took a lot of
    //  work with the stupid data sheet to get right. If you change them,
    //  things will probably break mysteriously.
    uint8_t bank1_config_0x00[][4] = {
            {0xe2, 0x01, 0x4b, 0x40}, // reserved (prescribed)
            {0x00, 0x00, 0x4b, 0xc0}, // reserved (prescribed)
            {0x02, 0x8c, 0xfc, 0xd0}, // reserved (prescribed)
            {0x41, 0x39, 0x00, 0x99}, // reserved (prescribed)
            {0x1b, 0x82, 0x96, 0xf9}, // 1 Mbps
            {0xa6, 0x0f, 0x06, 0x24}, // 1 Mbps
    };

    for (uint8_t i=0; i<6; i++) {
        rfm75_write_reg_buf(i, bank1_config_0x00[i], 4);
    }

    uint8_t bank1_config_0x0c[][4] = {
            {0x05, 0x73, 0x10, 0x00}, // 130 us mode (PLL settle time?)
            {0x00, 0x80, 0xb4, 0x36}, // reserved?
    };

    for (uint8_t i=0; i<2; i++) {
        rfm75_write_reg_buf(0x0c+i, bank1_config_0x0c[i], 4);
    }

    // Set the prescribed ramp curve:
    uint8_t bank1_config_0x0e[11] = {0xff, 0xff, 0xfe, 0xf7, 0xcf, 0x20, 0x81,
                                     0x04, 0x08, 0x20, 0x41};
    rfm75_write_reg_buf(0x0e, bank1_config_0x0e, 11);

    // Now we go back to bank 0, because that's the one we normally
    //  care about.

    rfm75_select_bank(0);

    // Flush our FIFOs just in case:
    CSN_LOW_START;
    rfm75spi_send_sync(FLUSH_RX);
    CSN_HIGH_END;
    CSN_LOW_START;
    rfm75spi_send_sync(FLUSH_TX);
    CSN_HIGH_END;

    // Enable our interrupts:
    RFM75_IRQ_IES |= RFM75_IRQ_PIN;
    RFM75_IRQ_IFG &= ~RFM75_IRQ_PIN;
    RFM75_IRQ_IE |= RFM75_IRQ_PIN;

    // And we're off to see the wizard!
    rfm75_enter_prx();
}

uint8_t rfm75_deferred_interrupt() {
    // RFM75 interrupt:
    uint8_t iv = rfm75_get_status();
    uint8_t ret = 0x00;

    if (iv & BIT4) { // no ACK interrupt
        // Clear the interrupt flag on the radio module:
        rfm75_write_reg(STATUS, BIT5);
        __no_operation();
        ret |= 0b100;
    }

    if (iv & BIT5 && rfm75_state == RFM75_TX_SEND) { // TX interrupt
        // We sent a thing.
        // The ISR already took us back to standby.
        // Clear the interrupt flag on the radio module:
        rfm75_write_reg(STATUS, BIT5);
        rfm75_state = RFM75_TX_DONE;
        // Return to PRX mode:
        rfm75_enter_prx();
        // It's a TX, so return 0b01:
        ret |= 0b01;
    }

    if (iv & BIT6 && rfm75_state == RFM75_RX_LISTEN) { // RX interrupt
        // We've received something.
        rfm75_state = RFM75_RX_READY;
        // Which pipe? (broadcast or unicast)
        uint8_t pipe = 0;
        pipe = (iv & 0b1110) ? 1 : 0;

        // Read the FIFO. No need to flush it; it's deleted when read.
        read_rfm75_cmd_buf(RD_RX_PLOAD, payload_in, RFM75_PAYLOAD_SIZE);
        // Clear the interrupt flag on the radio module.
        rfm75_write_reg(STATUS, BIT6);

        // There's one type of payloads that this is allowed to be:
        //     ==Broadcast==
        //     ===Unicast===

        // Payload is now allowed to go stale.
        // Assert CE: listen more.
        CE_ACTIVATE;
        rfm75_state = RFM75_RX_LISTEN;
        ret |= 0b10;
    }
    return ret;
}

#pragma vector=PORT1_VECTOR
__interrupt
void RFM_ISR(void)
{
    if (P1IV != RFMxIV_PxIFGx) {
        return;
    }
    f_rfm75_interrupt = 1;
    CE_DEACTIVATE; // stop listening or sending.
    LPM4_EXIT; // We may not be THIS sleepy, but this'll wake us from anything.
}
