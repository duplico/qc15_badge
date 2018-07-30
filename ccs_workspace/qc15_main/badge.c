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

#include "badge.h"

uint8_t badge_seen(uint16_t id) {
    return check_id_buf(id, badge_conf.badges_seen);
}

uint8_t badge_uploaded(uint16_t id) {
    return check_id_buf(id, badge_conf.badges_uploaded);
}

uint8_t badge_downloaded(uint16_t id) {
    return check_id_buf(id, badge_conf.badges_downloaded);
}

void load_badge_name(uint8_t *buf, uint16_t id) {
    // TODO TODO TODO TODO TODO TODO!!!!!
    if (!(global_flash_lockout&FLASH_LOCKOUT_READ)) {
        s25fs_read_data(
                buf,
                FLASH_ADDR_BADGE_NAMES + QC15_BADGE_NAME_LEN * id,
                QC15_BADGE_NAME_LEN
        );
    } else {
        char *t="BADGE";
        strcpy((char *)buf, t);
        return;
    }
}

void load_person_name(uint8_t *buf, uint16_t id) {
    uint32_t name_page_address = FLASH_ADDR_PERSON_NAMES + (0x010000 * (id / 20));
    uint8_t name_offset = (id % 20) * QC15_PERSON_NAME_LEN;
    if (!(global_flash_lockout&FLASH_LOCKOUT_READ)) {
        s25fs_read_data(buf, name_page_address + name_offset,
                        QC15_PERSON_NAME_LEN);
    } else {
        // TODO
    }
}

uint8_t set_badge_seen(uint16_t id, uint8_t *name) {
    if (id >= QC15_BADGES_IN_SYSTEM)
        return 0;
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

    uint32_t name_page_address = FLASH_ADDR_PERSON_NAMES + (0x010000 * (id / 20));
    uint8_t name_offset = (id % 20) * QC15_PERSON_NAME_LEN;

    uint8_t name_block[QC15_PERSON_NAME_LEN*20];

    load_person_name(name_block, id);


    if (!(global_flash_lockout & FLASH_LOCKOUT_WRITE) &&
            strcmp((const char *) name_block, (const char *)name)) {
        // Different name than what's saved. From here there are two options:
        //  we may have to do a big write, or a little write.
        if (name_block[0] == 0xFF) {
            // Little write! :-)
            s25fs_wr_en();
            s25fs_write_data(name_page_address + name_offset, name,
                             QC15_PERSON_NAME_LEN);
        } else {
            // Big write. :-(
            // We have to read the entire useful part of the block...
            s25fs_read_data(name_block, name_page_address,
                            QC15_PERSON_NAME_LEN*20);
            // Put our name into it in the right spot...
            memcpy(&name_block[name_offset], name, QC15_PERSON_NAME_LEN);
            // Clear the whole block...
            s25fs_wr_en();
            s25fs_erase_block_64kb(name_page_address);
            s25fs_block_while_wip(); // TODO: We should really use signals.
            s25fs_wr_en();
            // Then stick the new data in.
            s25fs_write_data(name_page_address, name_block, 222);
        }
    }

    save_config();
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

    save_config();

    return 1;
}

// TODO: Break out the save_config part
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

    save_config();
    return 1;
}


void save_config() {
    crc16_append_buffer((uint8_t *) (&badge_conf), sizeof(qc15conf)-2);

    if (!(global_flash_lockout & FLASH_LOCKOUT_WRITE)) {
        s25fs_wr_en();
        s25fs_erase_block_64kb(FLASH_ADDR_CONF_MAIN);
        s25fs_wr_en();
        s25fs_write_data(FLASH_ADDR_CONF_MAIN, (uint8_t *) (&badge_conf),
                         sizeof(qc15conf));
    }

    memcpy(&backup_conf, &badge_conf, sizeof(qc15conf));

    if (!(global_flash_lockout & FLASH_LOCKOUT_WRITE)) {
        s25fs_wr_en();
        s25fs_erase_block_64kb(FLASH_ADDR_CONF_BACKUP);
        s25fs_wr_en();
        s25fs_write_data(FLASH_ADDR_CONF_BACKUP, (uint8_t *) (&badge_conf),
                         sizeof(qc15conf));
    }

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

void generate_config() {
    // All we start from, here, is our ID.

    // The struct is no good. Zero it out.
    memset(&badge_conf, 0x00, sizeof(qc15conf));

    // TODO: Handle global_flash_lockout.
    //       Hopefully this won't come up, since this SHOULD(tm) only be
    //        called the once.

    // Load ID from flash:
    s25fs_read_data((uint8_t *)(&(badge_conf.badge_id)), FLASH_ADDR_ID_MAIN, 2);

    uint8_t sentinel;
    s25fs_read_data(&sentinel, FLASH_ADDR_sentinel, 1);

    // Check the handful of things we know to check in the flash to see if
    //  it's looking sensible to have a badge name. Otherwise, let's go with
    //  the classics.
    if (sentinel != FLASH_sentinel_BYTE ||
            badge_conf.badge_id >= QC15_BADGES_IN_SYSTEM ||
            badge_conf.badge_name[0] == 0xFF || badge_conf.badge_name[0] == 0)
    {
        badge_conf.badge_id = 115;
        char backup_name[] ="Skippy";
        strcpy((char *) &(badge_conf.badge_name[0]), backup_name);
    } else {
        load_badge_name(badge_conf.badge_name, badge_conf.badge_id);
    }

    // TODO: REMOVE!!!
    uint8_t initial_person_name[] = "AB";
    strcpy((char *) &(badge_conf.person_name[0]), (char *)initial_person_name);
    ///////////////////

    // Determine which segment we have (and therefore which parts)
    badge_conf.code_starting_part = (badge_conf.badge_id % 16) * 6;
    set_badge_seen(badge_conf.badge_id, initial_person_name);
    set_badge_uploaded(badge_conf.badge_id);
    set_badge_downloaded(badge_conf.badge_id);

    save_config();

    qc15_mode = QC15_MODE_GAME;
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
