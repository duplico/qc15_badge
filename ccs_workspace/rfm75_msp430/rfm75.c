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
rfbcpayload in_payload, out_payload, cascade_payload;

uint8_t rx_addr_p0[3] = {0xd6, 0xe7, 0x2a};
uint8_t tx_addr[3] = {0xd6, 0xe7, 0x2a};
uint8_t payload_in[RFM75_PAYLOAD_SIZE] = {0};
uint8_t payload_out[RFM75_PAYLOAD_SIZE] = {0};

uint8_t rfm75_state = RFM75_BOOT;

volatile uint8_t f_rfm75_interrupt = 0;

///////////////////////////////
// Bank initialization values:

#define BANK0_INITS 17
const uint8_t bank0_init_data[BANK0_INITS][2] = {
        { CONFIG, 0b00001111 }, //
        { 0x01, 0b00000000 }, //No auto-ack
        { 0x02, 0b00000001 }, //Enable RX pipe 0 and 1
        { 0x03, 0b00000001 }, //RX/TX address field width 3byte
        { 0x04, 0b00000000 }, //no auto-RT
        { 0x05, 0x53 }, //channel: 2400 + LS 7 bits of this field = channel (2.483)
        { 0x06, 0b00000111 }, //air data rate-1M,out power max, setup LNA gain high.
        { 0x07, 0b01110000 }, // Clear interrupt flags
        // 0x0a - RX_ADDR_P0 - 3 bytes
        // 0x0b - RX_ADDR_P1 - 3 bytes
        // 0x10 - TX_ADDR - 5 bytes
        { 0x11, RFM75_PAYLOAD_SIZE }, //Number of bytes in RX payload in data pipe0(32 byte)
        { 0x12, 0 }, //Number of bytes in RX payload in data pipe1 - disable
        { 0x13, 0 }, //Number of bytes in RX payload in data pipe2 - disable
        { 0x14, 0 }, //Number of bytes in RX payload in data pipe3 - disable
        { 0x15, 0 }, //Number of bytes in RX payload in data pipe4 - disable
        { 0x16, 0 }, //Number of bytes in RX payload in data pipe5 - disable
        { 0x17, 0 },
        { 0x1c, 0x00 }, // No dynamic packet length
        { 0x1d, 0b00000000 } // 00000 | DPL | ACK | DYN_ACK
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

uint8_t rfm75_read_byte(uint8_t cmd) {
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

uint8_t rfm75_post() {
    volatile uint8_t bank_one = rfm75_get_status() & 0x80; // Get MSB, which is active bank.
    send_rfm75_cmd(ACTIVATE_CMD, 0x53);
    volatile uint8_t bank_two = rfm75_get_status() & 0x80; // Get MSB, which is active bank.

    rfm75_select_bank(0); // Go back to the normal bank.

    if (bank_one == bank_two) {
        return 0;
    }
    return 1;
}

void rfm75_enter_prx() {
    rfm75_state = RFM75_RX_INIT;
    CE_DEACTIVATE;
    // Power up & enter PRX (Primary RX)
    rfm75_select_bank(0);
    rfm75_write_reg(CONFIG, CONFIG_MASK_TX_DS + CONFIG_MASK_TX_DS +
                            CONFIG_MASK_MAX_RT + CONFIG_EN_CRC +
                            CONFIG_CRCO_2BYTE + CONFIG_PWR_UP +
                            CONFIG_PRIM_RX);

    // Clear interrupts: STATUS=BIT4|BIT5|BIT6
    rfm75_write_reg(STATUS, BIT4|BIT5|BIT6);

    // Enter RX mode.
    CE_ACTIVATE;

    rfm75_state = RFM75_RX_LISTEN;
}

void rfm75_tx() {
    // Fill'er up:
    out_payload.proto_version = 0;
    out_payload.badge_addr = 0;
    out_payload.base_addr = 0xfd;
    out_payload.ttl = 2;
    out_payload.ink_id = 0x1a;
    out_payload.flags = 0x24;
    out_payload.seqnum = 41;
    out_payload.crc16 = 28108;
    memcpy(payload_out, &out_payload, RFM75_PAYLOAD_SIZE);
//    memset(payload_out, 0xdc, RFM75_PAYLOAD_SIZE);

    rfm75_state = RFM75_TX_INIT;
    CE_DEACTIVATE;
    rfm75_select_bank(0);
    rfm75_write_reg(CONFIG, 0b01011110);
    // Clear interrupts: STATUS=BIT4|BIT5|BIT6
    rfm75_write_reg(STATUS, BIT4|BIT5|BIT6);

    rfm75_state = RFM75_TX_FIFO;
    // Write the payload:
    send_rfm75_cmd_buf(WR_TX_PLOAD, payload_out, RFM75_PAYLOAD_SIZE);
    rfm75_state = RFM75_TX_SEND;
    CE_ACTIVATE;
    // Now we wait for an IRQ to let us know it's sent.
}

void rfm75_init()
{
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION); // TX
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION); // RX
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION); // CLK
    // Setup USCI_B0.
    EUSCI_B_SPI_initMasterParam ini = {0};
    ini.selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK;
    ini.clockSourceFrequency = CS_getSMCLK();
    ini.desiredSpiClock = 1000000;
    ini.msbFirst = EUSCI_B_SPI_MSB_FIRST;
    ini.clockPhase = EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
    ini.clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
    ini.spiMode = EUSCI_B_SPI_3PIN;

    EUSCI_B_SPI_initMaster(EUSCI_B0_BASE, &ini);
    EUSCI_B_SPI_enable(EUSCI_B0_BASE);

    // We're going totally synchronous on this; no interrupts at all.
    __delay_cycles(150000); // Delay more than 50ms.

    // Setup GPIO:
    // CSN (1.0):
    P1DIR |= BIT0; // CSN
    P1OUT |= BIT0; // init high.
    // CE (1.6):
    P1DIR |= BIT6;
    P1OUT &= ~BIT6;
    // IRQ (1.7):
    P1DIR &= ~BIT7;
    P1REN &= ~BIT7;
    P1SEL0 &= ~BIT7;
    P1SEL1 &= ~BIT7;
    // We'll wait on the interrupt enables though...

    // Let's start with bank 0:
    rfm75_select_bank(0);

    for(uint8_t i=0;i<BANK0_INITS;i++)
        rfm75_write_reg(bank0_init_data[i][0], bank0_init_data[i][1]);

    volatile uint8_t temp = 0;
    volatile uint8_t test = 0;
    for(uint8_t i=0;i<BANK0_INITS;i++) {
        temp = rfm75_read_byte(bank0_init_data[i][0]);
        test = temp == bank0_init_data[i][1];
        __no_operation();
    }

    // Next fill address buffers
    //  Reg 0x0a: 5 bytes RX0 addr (broadcast)
    //  Reg 0x10: 5 bytes TX0 addr (same as RX0)
    rfm75_write_reg_buf(RX_ADDR_P0, rx_addr_p0, 3); // broadcast
    rfm75_write_reg_buf(TX_ADDR, tx_addr, 3);

    // OK, that's bank 0 done. Next is bank 1.

    rfm75_select_bank(1);

    // Some of these go MOST SIGNIFICANT BYTE FIRST: (so we start with 0xE2.)
    //  (we show them here LEAST SIGNIFICANT BYTE FIRST because we
    //   reverse everything we send.)
    // Like this:
    uint8_t bank1_config_0x00[][4] = {
            {0xe2, 0x01, 0x4b, 0x40}, // reserved (prescribed)
            {0x00, 0x00, 0x4b, 0xc0}, // reserved (prescribed)
            {0x02, 0x8c, 0xfc, 0xd0}, // reserved (prescribed)
            {0x41, 0x39, 0x00, 0x99}, // reserved (prescribed)
            {0x1b, 0x82, 0x96, 0xf9}, // 1 Mbps // The user guide flips it for us.
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

    uint8_t bank1_config_0x0e[11] = {0xff, 0xff, 0xfe, 0xf7, 0xcf, 0x20, 0x81, 0x04, 0x08, 0x20, 0x41};
    // {0x41, 0x20, 0x08, 0x04, 0x81, 0x20, 0xcf, 0xf7, 0xfe, 0xff, 0xff}; // ramp curve, prescribed
    rfm75_write_reg_buf(0x0e, bank1_config_0x0e, 11);

    // Now we go back to bank 0, because that's the one we normally
    //  care about.

    rfm75_select_bank(0);

    // Enable our interrupts:
    P1IES |= BIT7;
    P1IFG &= ~BIT7;
    P1IE |= BIT7;

    // And we're off to see the wizard!

    CSN_LOW_START;
    rfm75spi_send_sync(FLUSH_RX);
    CSN_HIGH_END;
    CSN_LOW_START;
    rfm75spi_send_sync(FLUSH_TX);
    CSN_HIGH_END;

    rfm75_enter_prx();
    __no_operation();
}

void rfm75_deferred_interrupt() {
    // RFM75 interrupt:
    uint8_t iv = rfm75_get_status();

    if (iv & BIT5 && rfm75_state == RFM75_TX_SEND) { // TX interrupt
        // We sent a thing.
        // The ISR already took us back to standby.
        // Clear interrupt, return to PRX
        rfm75_write_reg(STATUS, BIT5);
        rfm75_state = RFM75_TX_DONE;
        rfm75_enter_prx();
    }

    if (iv & BIT6 && rfm75_state == RFM75_RX_LISTEN) { // RX interrupt
        // We've received something.
        rfm75_state = RFM75_RX_READY;
        // Which pipe?
        // Read the FIFO. No need to flush it; deleted when read.
        read_rfm75_cmd_buf(RD_RX_PLOAD, payload_in, RFM75_PAYLOAD_SIZE);
        // Clear the interrupt.
        rfm75_write_reg(STATUS, BIT6);
        memcpy(&in_payload, &payload_in, RFM75_PAYLOAD_SIZE);

        // There's one type of payloads that this is allowed to be:
        //     ==Broadcast==

        // Payload is now allowed to go stale.
        // Assert CE: listen more.
        CE_ACTIVATE;
        rfm75_state = RFM75_RX_LISTEN;
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void RFM_ISR(void)
{
    if (P1IV != P1IV_P1IFG7) {
        return;
    }
    f_rfm75_interrupt = 1;
    rfm75_deferred_interrupt();
    CE_DEACTIVATE; // stop listening or sending.
    LPM4_EXIT; // We may not be this sleepy, but this'll wake us regardless.
}
