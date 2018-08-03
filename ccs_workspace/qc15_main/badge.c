/// Broadly applicable state and storage management module for Queercon 15.
/**
 ** This is for the main MCU on the badge, which is an MSP430FR5972. This file
 ** handles all of the definitions ANY persistent data on the badge.
 **
 ** And it implements the following types of functions:
 **
 ** * Application-level management of persistently stored values
 ** * Application-level time management
 **
 ** \file badge.c
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <msp430.h>

#include "qc15.h"
#include "util.h"

#include "flash_layout.h"
#include "ipc.h"
#include "s25fs.h"
#include "lcd111.h"
#include "codes.h"
#include "leds.h"
#include "led_animations.h"

#include "badge.h"

// NOT persistent:
uint8_t unlock_radio_status = 0;
uint32_t disable_event_at = 0;

// PERSISTENT won't let these persist between badge flashings. However, we're
//  not putting them in a consistent place this time, so we can't guarantee
//  that NOINIT would help us here, either. We'll have to very carefully
//  extract these from memory first if we wanted to wipe a badge's brain.
#pragma PERSISTENT(badge_conf)
qc15conf badge_conf = {0};
#pragma PERSISTENT(backup_conf)
qc15conf backup_conf = {0};

// Badge names don't need to be persistent - they're just consts:
#include "badge_names.c"

#pragma PERSISTENT(person_names)
char person_names[QC15_BADGES_IN_SYSTEM][QC15_PERSON_NAME_LEN] = {0};

#pragma PERSISTENT(stored_state_id)
uint16_t stored_state_id = 0;
#pragma PERSISTENT(last_state_id)
uint16_t last_state_id = 0;
#pragma PERSISTENT(game_curr_state_id)
uint16_t game_curr_state_id = 0;

#pragma PERSISTENT(led_ring_anim_bg)
/// Pointer to saved animation (if we're doing a temporary one).
const led_ring_animation_t *led_ring_anim_bg = 0;

#pragma PERSISTENT(led_ring_anim_pad_loops_bg)
/// Saved value of `led_ring_anim_pad_loops` for backgrounds.
uint8_t led_ring_anim_pad_loops_bg = 0;

#pragma PERSISTENT(led_anim_type_bg)
/// Saved value of `led_anim_type` for backgrounds.
uint8_t led_anim_type_bg = 0;

uint8_t badge_seen(uint16_t id) {
    return check_id_buf(id, badge_conf.badges_seen);
}

uint8_t badge_uploaded(uint16_t id) {
    return check_id_buf(id, badge_conf.badges_uploaded);
}

uint8_t badge_downloaded(uint16_t id) {
    return check_id_buf(id, badge_conf.badges_downloaded);
}

uint8_t set_badge_seen(uint16_t id, uint8_t *name) {
    if (id >= QC15_EVENT_ID_START && id <= QC15_EVENT_ID_END) {
        decode_event(id - QC15_EVENT_ID_START);
        return 0;
    } else if (id >= QC15_BADGES_IN_SYSTEM) {
        return 0;
    }

    // If we're here, it's a badge.

    // Let's update its name.
    memcpy(person_names[id], name, QC15_PERSON_NAME_LEN-1);
    person_names[id][QC15_PERSON_NAME_LEN-1]=0x00;

    if (badge_seen(id)) {
        return 0;
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

    save_config(1);
    return 1;
}

uint8_t set_badge_uploaded(uint16_t id) {
    if (id >= QC15_BADGES_IN_SYSTEM)
        return 0;
    if (badge_uploaded(id)) {
        return 0;
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

    decode_upload(id);
    save_config(1);

    return 1;
}

uint8_t set_badge_downloaded(uint16_t id) {
    if (id >= QC15_BADGES_IN_SYSTEM)
        return 0;
    if (badge_downloaded(id)) {
        return 0;
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

    decode_download(id);
    save_config(1);
    return 1;
}

void save_config(uint8_t send_to_radio) {
    badge_conf.last_clock = qc_clock.time;
    crc16_append_buffer((uint8_t *) (&badge_conf), sizeof(qc15conf)-2);
    memcpy(&backup_conf, &badge_conf, sizeof(qc15conf));

    if (unlock_radio_status && send_to_radio) {
        // And, update our friend the radio MCU:
        // (spin until the send is successful)
        ipc_tx_op_buf(IPC_MSG_STATS_UPDATE, (uint8_t *) &badge_conf, sizeof(qc15status));
    }
}

uint8_t is_handler(uint16_t id) {
    return ((id <= QC15_HANDLER_LAST) &&
            (id >= QC15_HANDLER_START));
}

uint8_t is_uber(uint16_t id) {
    return (id < QC15_UBER_COUNT);
}

void generate_config() {
    // All we start from, here, is our ID.

    // The struct is no good. Zero it out.
    memset(&badge_conf, 0x00, sizeof(qc15conf));

    uint16_t id=0xffff;
    uint16_t id_bak=0xffff;

    // Handle global_flash_lockout.
    //       Hopefully this won't come up, since this SHOULD(tm) only be
    //        called the once.

    if(!(global_flash_lockout & FLASH_LOCKOUT_READ)) {
        // Load ID from flash:
        s25fs_read_data((uint8_t *)&id, FLASH_ADDR_ID_MAIN, 2);
        s25fs_read_data((uint8_t *)&id_bak, FLASH_ADDR_ID_BACKUP, 2);
    }


    // If we got a bad ID from the flash, we can't trust ANYTHING.
    if ((global_flash_lockout & FLASH_LOCKOUT_READ) ||
            (id >= QC15_BADGES_IN_SYSTEM) || (id != id_bak)) {
        lcd111_set_text(LCD_TOP, "ID corrupt. Reboot.");
        while (1); // spin forever.
    }

    badge_conf.badge_id = id;

    memcpy(badge_conf.badge_name, badge_names[badge_conf.badge_id],
           QC15_BADGE_NAME_LEN);

    // TODO: REMOVE!!!
    uint8_t initial_person_name[] = "AB";
    strcpy((char *) &(badge_conf.person_name[0]), (char *)initial_person_name);
    ///////////////////

    badge_conf.last_clock = DEFAULT_CLOCK_TIME;
    qc_clock.time = badge_conf.last_clock;

    // Determine which segment we have (and therefore which parts)
    badge_conf.code_starting_part = (badge_conf.badge_id % 16) * 6;
    set_badge_seen(badge_conf.badge_id, initial_person_name);
    set_badge_uploaded(badge_conf.badge_id);
    set_badge_downloaded(badge_conf.badge_id);
}

uint8_t config_is_valid() {
    if (!crc16_check_buffer((uint8_t *) (&badge_conf), sizeof(qc15conf)-2))
        return 0;

    if (badge_conf.badge_id > QC15_BADGES_IN_SYSTEM)
        return 0;

    return 1;
}

uint8_t flag_unlocked(uint8_t flag_num) {
    if (flag_num >= FLAG_COUNT) {
        return 0;
    }

    return badge_conf.flag_unlocks & (0x00000001 << flag_num);
}

void unlock_flag(uint8_t flag_num) {
    if (flag_num >= FLAG_COUNT || flag_unlocked(flag_num))
        return;

    badge_conf.flag_unlocks |= (0x00000001 << flag_num);
    save_config(0);
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
