/*
 * text_entry.c
 *
 *  Created on: Jul 23, 2018
 *      Author: george
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "qc15.h"
#include "lcd111.h"
#include "loop_signals.h"

#include "badge.h"
#include "textentry.h"

// None of this needs to be persisted, because we can only ever startup into
//  the GAME or COUNTDOWN modes, really.

char *textentry_dest;
char curr_text[25];
uint8_t text_entry_cursor_pos;
uint8_t textentry_len;
uint8_t textentry_exit_mode;

uint8_t text_entry_in_progress = 0;

#define CH_MUSIC 0x96
#define CH_HEART 0x9D
#define CH_ENTER 0x17

/// The len here does NOT include a null terminator.
void textentry_begin(char *destination, uint8_t len, uint8_t start_populated,
                     uint8_t show_instructions) {
    if (!len) {
        return; // INVALID INVOCATION OF THIS FUNCTION
    }
    textentry_dest = destination;
    textentry_len = len;
    text_entry_cursor_pos = 0;
    text_entry_in_progress = 1;
    textentry_exit_mode = qc15_mode;
    qc15_mode = QC15_MODE_TEXTENTRY;

    memset(curr_text, 0, len+1);

    if (start_populated) {
        memcpy(curr_text, textentry_dest, len);
    }

    if (!curr_text[0]) {
        curr_text[0] = 'A';
    }

    if (show_instructions) {
        lcd111_cursor_type(LCD_TOP, LCD111_CURSOR_NONE);
        lcd111_set_text(LCD_TOP, "\xABSelect:\x11\x1e\x1f\x10 | Submit:\x17\xBB");
    }

    lcd111_set_text(LCD_BTM, curr_text);
    lcd111_cursor_type(LCD_BTM, LCD111_CURSOR_INVERTING);
    lcd111_cursor_pos(LCD_BTM, 0);
}

void next_char() {
    /*
     * Allowed characters:
     * 0x17 - ENTER
     * 0x30 - 0x3A - numbers
     * 0x41 - 0x5A - upper
     * 0x61 - 0x7A - lower
     * 0x20 - 0x24 symbols (' ' .. '$')
     * 0x3F - 0x40 - symbols ('?', '@')
     * CH_MUSIC - music
     * CH_HEART - heart
     */
    uint8_t ch = curr_text[text_entry_cursor_pos];

    if (
            (ch >= 'A' && ch < 'Z') ||
            (ch >= 'a' && ch < 'z') ||
            (ch >= '0' && ch < '9') ||
            ((ch >= ' ' && ch < '$') || (ch >= '?' && ch < '@'))
    ) {
        ch++;
    }
    else if (ch == 'Z')
        ch = 'a';
    else if (ch == 'z')
        ch = '0';
    else if (ch == '9')
           ch = ' ';
    else if (ch == '$')
        ch = '?';
    else if (ch == '@')
        ch = CH_MUSIC;
    else if (ch == CH_MUSIC)
        ch = CH_HEART;
    else if (ch == CH_HEART)
        ch = CH_ENTER; // ENTER
    else if (ch == CH_ENTER) // enter
        ch = 'A';

    curr_text[text_entry_cursor_pos] = ch;

    if (text_entry_cursor_pos == 0 && ch == CH_ENTER) { // no 0-len strings
        next_char();
    }
}

void prev_char() {
    /*
     * Allowed characters:
     * 0x17 - ENTER
     * 0x30 - 0x3A - numbers
     * 0x41 - 0x5A - upper
     * 0x61 - 0x7A - lower
     * 0x20 - 0x24 symbols (' ' .. '$')
     * 0x3F - 0x40 - symbols ('?', '@')
     * CH_MUSIC - music
     * CH_HEART - heart
     */
    uint8_t ch = curr_text[text_entry_cursor_pos];
    if (
            (ch > 'A' && ch <= 'Z') ||
            (ch > 'a' && ch <= 'z') ||
            (ch > '0' && ch <= '9') ||
           ((ch > ' ' && ch <= '$') || (ch > '?' && ch <= '@'))
    ) {
        ch--;
    }
    else if (ch == 'A') // A -> ENTER
        ch = 0x17;
    else if (ch == 'a')
        ch = 'Z';
    else if (ch == '0')
        ch = 'z';
    else if (ch == ' ')
        ch = '9';
    else if (ch == '?')
        ch = '$';
    else if (ch == CH_MUSIC)
        ch = '@';
    else if (ch == CH_HEART)
        ch = CH_MUSIC;
    else if (ch == CH_ENTER) // enter
        ch = CH_HEART;

    curr_text[text_entry_cursor_pos] = ch;

    if (text_entry_cursor_pos == 0 && ch == CH_ENTER) { // no 0-len strings
        prev_char();
    }
}

void textentry_update_single_char() {
    lcd111_cursor_pos(LCD_BTM, text_entry_cursor_pos);
    uint8_t blanking = 0;
    for (uint8_t i=text_entry_cursor_pos; i<=textentry_len; i++) {
        if (!curr_text[i]) // null terminator
            blanking = 1;
        if (blanking) {
            lcd111_put_char(LCD_BTM, ' ');
        } else {
            lcd111_put_char(LCD_BTM, curr_text[i]);
            if (curr_text[i] == CH_ENTER)
                blanking = 1;
        }
    }

    lcd111_cursor_pos(LCD_BTM, text_entry_cursor_pos);
}

void textentry_complete() {
    curr_text[text_entry_cursor_pos] = 0; // Null term at the ENTER.
    lcd111_cursor_type(LCD_BTM, LCD111_CURSOR_NONE);
    memcpy(textentry_dest, curr_text, textentry_len+1);
    lcd111_clear_nodelay(LCD_BTM);
    lcd111_clear_nodelay(LCD_TOP);
    qc15_set_mode(textentry_exit_mode);
    text_entry_in_progress = 0;
    // Do a SAVE, out of an abundance of caution.
    save_config(1);
}

void textentry_handle_loop() {
    if (!s_clock_tick)
        return; // Only care about clock ticks (and therefore buttons)

    if (s_up && (text_entry_cursor_pos < textentry_len)) {
        next_char();
        textentry_update_single_char();
    } else if (s_down && (text_entry_cursor_pos < textentry_len)) {
        prev_char();
        textentry_update_single_char();
    } else if (s_right) {
        // set cursor
        if (curr_text[text_entry_cursor_pos] == CH_ENTER) {
            // We selected OK!
            textentry_complete();
            return;
        }
        if (text_entry_cursor_pos < textentry_len) {
            text_entry_cursor_pos++;
        }
        else {
            text_entry_cursor_pos = textentry_len;
            curr_text[text_entry_cursor_pos] = CH_ENTER;
        }

        if (!curr_text[text_entry_cursor_pos]) {
            curr_text[text_entry_cursor_pos] = CH_ENTER;
        }
        textentry_update_single_char();

    } else if (s_left) {
        if (text_entry_cursor_pos) {
            text_entry_cursor_pos--;
            lcd111_cursor_pos(LCD_BTM, text_entry_cursor_pos);
        }
    }
}
