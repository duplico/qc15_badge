/// The implementation of the multi-badge codebreaking game.
/**
 *  \file codes.c
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */
#include <stdint.h>
#include <stdlib.h>

#include <msp430.h>

#include "qc15.h"
#include "util.h"

#include "badge.h"

#define PART_SIZE 80

uint8_t is_solved(uint8_t code_id) {
//    uint8_t code_part_unlocks[6][CODE_SEGMENT_REP_LEN];
    uint8_t bits_decoded;
    bits_decoded = buffer_rank(badge_conf.code_part_unlocks[code_id], 6);
    return bits_decoded == 80;
}

uint8_t part_id_downloaded(uint16_t id) {
    // This is PART 0,1,4,5
    // Lots of options here:
    if (!is_solved(0) && (is_uber(id) || is_handler(id))) {
        // 1. Is it uber/handler? (part 0)
        return 0;
    }
    // If not, we can keep checking the following:

    // If not, is it the same as us?
    if (!is_solved(5) && (badge_conf.badge_id % 16 == id % 16)) {
        // If it's the same, and we still have characters to decode in our
        //  "same" part, then pick that one.
        return 5;
    } else if (badge_conf.badge_id % 16 == id % 16) {
        // Otherwise, this one has no information that we don't already have.
        //  return our version of NULL.
        return 0xff; // Nothing to do.
    }
    // 3. If it's different, choose between parts 1 and 4.
    // If one or both are totally solved already, that's pretty easy!
    //  Just pick the other one, or none.
    if (is_solved(1) && is_solved(4)) {
        return 0xff;
    } else if (is_solved(1)) {
        return 4;
    } else if (is_solved(4)) {
        return 1;
    }

    // If we're here, both have some free characters. Pick one at random to
    //  decode part of.
    if (rand() & 0x01) {
        return 1;
    } else {
        return 4;
    }
}

void decode_random_chars(uint8_t part_id, uint8_t chars_to_decode) {
    uint8_t chars_left;
    chars_left=   PART_SIZE
                - buffer_rank(badge_conf.code_part_unlocks[part_id], 6);

    if (chars_to_decode < chars_left) {
        chars_to_decode = chars_left;
    }

    // This is hideously inefficient.
    while (chars_to_decode) {
        uint8_t decode_char = (rand() % chars_left);
        for (uint8_t i=0; i<PART_SIZE; i++) {
            if (check_id_buf(i, badge_conf.code_part_unlocks[part_id])) {
                // unlocked char.
                continue;
            }
            // we're at a LOCKED char!
            if (decode_char) {
                // Still more to go. Decrement and continue the for loop.
                decode_char--;
            } else {
                // We're at the character we wanted.
                set_id_buf(i, badge_conf.code_part_unlocks[part_id]);
                break; // Back to the while loop - new random and for loop.
            }
        }
        chars_to_decode--;
    }
    save_config();
}

void decode_download(uint16_t downloaded_id) {
    uint8_t part_id;
    uint8_t chars_to_decode;

    part_id = part_id_downloaded(downloaded_id);
    if (part_id == 0xff) {
        return;
    }

    // Ok, so if we're here, we know that we can decode SOME NUMBER OF CHARS
    //  in part_id.
    if (part_id == 0 || part_id == 5)
        chars_to_decode = 5;
    else
        chars_to_decode = 3;

    decode_random_chars(part_id, chars_to_decode);

}

void decode_upload() {
    // This is PART 3!
    decode_random_chars(3, 3);
}

#define EVENT_FRI_MIXER
#define EVENT_BADGETALK
#define EVENT_SAT_MIXER
#define EVENT_SAT_PARTY
#define EVENT_SAT_KARAOKE
#define EVENT_CLOSING
#define EVENT_FREEZER

void decode_event(uint8_t event_id) {
    // This is PART 2!
    //  It's more of a special case. There are 7 events, each of which
    //  unlocks 12 characters.
    switch(event_id) {

    }
}
