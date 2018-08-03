/*
 * qc15.h
 *
 *  Created on: Jul 7, 2018
 *      Author: george
 */

#ifndef QC15_H_
#define QC15_H_

#include "msp430.h"
#include <stdint.h>

#define QC15_CRC_SEED 0x5321

// Badge type counts: (see qc15_notes/inhuman_list.txt)
/// Number of uber badges (they are 0-origined).
#define QC15_UBER_COUNT 15
/// First ID of a handler badge.
#define QC15_HANDLER_START 8
/// Number of handlers (23 is the LAST handler badge).
#define QC15_HANDLER_COUNT 16
#define QC15_HANDLER_LAST (QC15_HANDLER_START+QC15_HANDLER_COUNT-1)

// The actual lengths are 10, but we add 1 because of the null terminator.
#define QC15_PERSON_NAME_LEN 11
#define QC15_BADGE_NAME_LEN 11

// Number of radio flush intervals to keep a badge in the system:
#define RADIO_GD_INTERVAL 10

// Status of the power switch:
#define POWER_SW_ON 1
#define POWER_SW_OFF 0

#define EVENT_FRI_MIXER 0
#define EVENT_BADGETALK 1
#define EVENT_SAT_MIXER 2
#define EVENT_SAT_PARTY 3
#define EVENT_SAT_KARAOKE 4
#define EVENT_CLOSING 5
#define EVENT_FREEZER 6

#define FLAG_FREEZER 18
#define FLAG_RAINBOW 0

#define QC15_BADGES_IN_SYSTEM 450
#define QC15_BASE_ID QC15_BADGES_IN_SYSTEM
#define QC15_EVENT_ID_START (QC15_BADGES_IN_SYSTEM + 1) // 451
#define QC15_EVENT_ID_END (QC15_BADGES_IN_SYSTEM + 1 + EVENT_FREEZER) // 457
#define QC15_CONTROL_ID (QC15_EVENT_ID_END + 1) // 458
#define QC15_HOSTS_IN_SYSTEM (QC15_CONTROL_ID+1) // 459

#define CODE_SEGMENT_REP_LEN 10

#define LPM_BITS LPM0_bits
#define LPM LPM0
#define LPM_EXIT LPM0_EXIT

#define QC15_MODE_BOOT      0
#define QC15_MODE_COUNTDOWN 1
#define QC15_MODE_STATUS    2
#define QC15_MODE_SLEEP     3
#define QC15_MODE_TEXTENTRY 4
#define QC15_MODE_TEMP      5
#define QC15_MODE_FLASH_BROKEN 6
#define QC15_MODE_CONTROLLER 7

#define QC15_MODE_GAME      64
#define QC15_MODE_GAME_CHECKNAME 65
#define QC15_MODE_GAME_CONNECT 66

/// The time after 8pm Wednesday that the badge wakes up.
/**
 ** According to our badges, queercon begins at 8pm Wednesday night. The clock
 ** counts in 1/32 of seconds. So, here's a quick translation:
 **
 **    ticks      unit
 **    -----      ----
 **       32       1 second
 **     1920       1 minute
 **   115200       1 hour
 **   172800       1.5 hours
 **  2764800       1 day (8pm Thursday)
 **  5529600       2 days (8pm Friday, pool party)
 **  8294400       3 days (8pm Saturday, karaoke/dance party)
 ** 10368000       CLOSING CEREMONIES
 ** 11059200       4 days (8pm Sunday, end secret party)
 **536870912       6 months (failsafe wrap)
 **
 */
#define QC_START_TIME 172800
#define QC_PARTY_TIME 8294400
#define QC_CLOSING_TIME 10368000

// !!!!!! SHOULD BE ZERO:::::::\/
#define DEFAULT_CLOCK_TIME 0
//#define DEFAULT_CLOCK_TIME (QC_START_TIME - 57600) // 30 minutes unless reset
//#define DEFAULT_CLOCK_TIME (QC_START_TIME - 640)

/// The main config struct, which will go in both the SPI flash and in FRAM.
typedef struct {
    uint16_t badge_id;
    char person_name[QC15_PERSON_NAME_LEN];
    char badge_name[QC15_PERSON_NAME_LEN];
    uint8_t code_starting_part;
    uint8_t code_part_unlocks[6][CODE_SEGMENT_REP_LEN];
    uint16_t badges_seen_count, badges_downloaded_count, badges_uploaded_count;
    uint8_t ubers_seen_count, ubers_downloaded_count, ubers_uploaded_count;
    uint8_t handlers_seen_count, handlers_downloaded_count,
            handlers_uploaded_count;
    uint32_t last_clock;
    uint8_t active;
    uint8_t event_beacon;
    uint8_t event_id;
    //////// Above this line should EXACTLY MATCH qc15status. ////////////////
    uint8_t badges_seen[57];
    uint8_t badges_downloaded[57];
    uint8_t badges_uploaded[57];
    uint32_t flag_unlocks;
    uint32_t ubers_seen, ubers_downloaded, ubers_uploaded;
    uint32_t handlers_seen, handlers_downloaded, handlers_uploaded;
    uint8_t file_lights_on : 1;
    uint8_t freezer_done : 1;
    uint8_t countdown_over : 1;
    uint8_t freq_set : 1;
    uint8_t freq_center;
    uint16_t crc16;
} qc15conf;

/// Struct for the radio MCU's ephemeral status, sent by the main MCU.
/**
 ** Note that this struct must be EXACTLY the same as the beginning of the
 ** `qc15conf` struct, so we can just do a memcpy.
 */
typedef struct {
    uint16_t badge_id;
    char person_name[QC15_PERSON_NAME_LEN];
    char badge_name[QC15_PERSON_NAME_LEN];
    uint8_t code_starting_part;
    uint8_t code_part_unlocks[6][CODE_SEGMENT_REP_LEN];
    uint16_t badges_seen_count, badges_downloaded_count, badges_uploaded_count;
    uint8_t ubers_seen_count, ubers_downloaded_count, ubers_uploaded_count;
    uint8_t handlers_seen_count, handlers_downloaded_count,
            handlers_uploaded_count;
    uint32_t last_clock;
    uint8_t active;
    uint8_t event_beacon;
    uint8_t event_id;
} qc15status;

typedef struct {
    volatile uint8_t authoritative;
    volatile uint8_t fault;
    volatile uint32_t time;
} qc_clock_t;

/// Our 1/32 second clock, which is ephemeral and sourced from the radio MCU.
/**
 ** Both MCUs have this clock variable, but the radio MCU's is authoritative
 ** (because it has the watch crystal on it).
 */
extern volatile qc_clock_t qc_clock;

#define SMCLK_FREQ_KHZ 1000
#define SMCLK_FREQ_HZ 1000000

// The following items are specific to the RADIO MCU:
#ifdef __MSP430FR2422__
#define MCLK_FREQ_KHZ 1000
#define MCLK_FREQ_HZ 1000000
extern qc15status badge_status;
#endif

#ifdef __MSP430FR2433__
#define MCLK_FREQ_KHZ 1000
#define MCLK_FREQ_HZ 1000000
#endif

#define FLASH_LOCKOUT_READ  0b01
#define FLASH_LOCKOUT_WRITE 0b10

// The following are specific to the MAIN MCU:
#ifdef __MSP430FR5972__
#define MCLK_FREQ_KHZ 8000
#define MCLK_FREQ_HZ 8000000
extern uint16_t badges_nearby;
extern uint8_t global_flash_lockout;
extern uint8_t qc15_mode;
void save_config(uint8_t send_to_radio);
#endif


void handle_ipc_rx(uint8_t *);

extern uint8_t power_switch_status;

#endif /* QC15_H_ */
