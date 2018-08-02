/// The implementation of status and control menus.
/**
 **
 ** \file menu.c
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <msp430fr5972.h>
#include <driverlib.h>
#include <s25fs.h>
#include <s25fs.h>
#include <textentry.h>
#include "qc15.h"

#include "lcd111.h"
#include "ht16d35b.h"
#include "ipc.h"
#include "leds.h"
#include "game.h"
#include "util.h"
#include "main_bootstrap.h"
#include "flash_layout.h"
#include "badge.h"
#include "codes.h"
#include "loop_signals.h"
#include "led_animations.h"
#include "menu.h"

#define MENU_EXIT 0

#define MENU_STATUS_SEL_SET_NAME   1
#define MENU_STATUS_SEL_SET_LIGHTS 2
#define MENU_STATUS_SEL_PART0 3
#define MENU_STATUS_SEL_PART1 4
#define MENU_STATUS_SEL_PART2 5
#define MENU_STATUS_SEL_PART3 6
#define MENU_STATUS_SEL_PART4 7
#define MENU_STATUS_SEL_PART5 8
#define MENU_STATUS_SEL_SEEN 9
#define MENU_STATUS_SEL_DOWNLOADED 10
#define MENU_STATUS_SEL_UPLOADED 11
#define MENU_STATUS_SEL_RADIOCAL 12
#define MENU_STATUS_MAX 12

uint8_t menu_sel = 0;
uint8_t saved_mode;

uint8_t curr_flag = 0;

void leave_menu();

void status_render_choice() {
    char text[25] = {0,};
    uint8_t num = 0;
    uint8_t code_part = 0;

    switch(menu_sel) {
    case MENU_EXIT:
        // Return to the game.
        sprintf(text, "%s Badge Status", badge_conf.badge_name);
        lcd111_set_text(LCD_TOP, text);
        draw_text(LCD_BTM, "Exit...", 1);
        return;
    case MENU_STATUS_SEL_SET_NAME:
        sprintf(text, "Badgeholder %s", badge_conf.person_name);
        lcd111_set_text(LCD_TOP, text);
        draw_text(LCD_BTM, "Change name...", 1);
        return;
    case MENU_STATUS_SEL_SET_LIGHTS:
        sprintf(text, "\x11  %s", all_animations[curr_flag].name);
        for (uint8_t i=strlen(text); i<23; i++) {
            text[i] = ' ';
        }
        text[23] = 0x10; // ending right arrow
        lcd111_set_text(LCD_TOP, text);
        draw_text(LCD_BTM, "(Set lights)", 1);
        break;
    case MENU_STATUS_SEL_PART0: // fall through
    case MENU_STATUS_SEL_PART1: // fall through
    case MENU_STATUS_SEL_PART2: // fall through
    case MENU_STATUS_SEL_PART3: // fall through
    case MENU_STATUS_SEL_PART4: // fall through
    case MENU_STATUS_SEL_PART5: // fall through
        code_part = menu_sel - MENU_STATUS_SEL_PART0;
        if (badge_conf.code_starting_part+code_part < 0x10) {
            sprintf(text, "(Part 0x0%x progress)",
                    badge_conf.code_starting_part+code_part);
        } else {
            sprintf(text, "(Part 0x%x progress)",
                    badge_conf.code_starting_part+code_part);
        }
        draw_text(LCD_BTM, text, 1);
        sprintf(text, " [____________________] ");
        for (uint8_t i=0; i<CODE_SEGMENT_REP_LEN*2; i++) {
            num = byte_rank(((i&0x01) ? 0x0F : 0xF0) &
                             badge_conf.code_part_unlocks[code_part][i/2]);
            if (num == 4) {
                text[2+i] = 0x16;
            } else if (num) {
                text[2+i] = 0xBB + num;
            }
        }
        lcd111_set_text(LCD_TOP, text);

        break; // no action
    case MENU_STATUS_SEL_SEEN:
        sprintf(text, "(Badges seen: %d)", badge_conf.badges_seen_count);
        draw_text(LCD_BTM, text, 1);
        sprintf(text, "Uber:%d Staff/Handler:%d", badge_conf.ubers_seen_count,
                badge_conf.handlers_seen_count);
        lcd111_set_text(LCD_TOP, text);
        break;
    case MENU_STATUS_SEL_DOWNLOADED:
        sprintf(text, "(Badges downloaded %d)",
                badge_conf.badges_downloaded_count);
        draw_text(LCD_BTM, text, 1);
        sprintf(text, "Uber:%d Staff/Handler:%d",
                badge_conf.ubers_downloaded_count,
                badge_conf.handlers_downloaded_count);
        lcd111_set_text(LCD_TOP, text);
        break;
    case MENU_STATUS_SEL_UPLOADED:
        sprintf(text, "(Downloaded me: %d)", badge_conf.badges_uploaded_count);
        draw_text(LCD_BTM, text, 1);
        sprintf(text, "Uber:%d Staff/Handler:%d",
                badge_conf.ubers_uploaded_count,
                badge_conf.handlers_uploaded_count);
        lcd111_set_text(LCD_TOP, text);
        break;
    case MENU_STATUS_SEL_RADIOCAL:
        draw_text(LCD_BTM, "(Radio Calibration)", 1);
        if (badge_conf.freq_set) {
            sprintf(text, "Freq: 2400 + %d",
                    badge_conf.freq_center);
        } else {
            sprintf(text, "Not set!");
        }
        lcd111_set_text(LCD_TOP, text);
        break;
    default:
        break;
    }
}

void status_handle_loop() {
    if (s_up) {
        if (menu_sel == MENU_STATUS_MAX)
            menu_sel = 0;
        else
            menu_sel++;
        // Don't show the flags if we don't have any available:
        if (menu_sel == MENU_STATUS_SEL_SET_LIGHTS && !badge_conf.flag_unlocks)
            menu_sel++;
        status_render_choice();
    } else if (s_down) {
        if (menu_sel == 0)
            menu_sel = MENU_STATUS_MAX;
        else
            menu_sel--;
        if (menu_sel == MENU_STATUS_SEL_SET_LIGHTS && !badge_conf.flag_unlocks)
            menu_sel--;
        status_render_choice();
    } else if (s_left) {
        if (menu_sel == MENU_STATUS_SEL_SET_LIGHTS) {
            // go back
            do {
                if (curr_flag == 0)
                    curr_flag = FLAG_COUNT-1;
                else
                    curr_flag--;
            } while (!flag_unlocked(curr_flag));
            led_set_anim(&all_animations[curr_flag], 0, 0xff, 0);
            status_render_choice();
        }
    } else if (s_right) {
        // Select.
        switch(menu_sel) {
        case MENU_EXIT:
            // Return to the game.
            leave_menu();
            return;
        case MENU_STATUS_SEL_SET_NAME:
            textentry_begin(badge_conf.person_name, 10, 1, 1);
            return;
        case MENU_STATUS_SEL_SET_LIGHTS:
            // go forward
            do {
                if (curr_flag == FLAG_COUNT)
                    curr_flag = 0;
                else
                    curr_flag++;
            } while (!flag_unlocked(curr_flag));
            led_set_anim(&all_animations[curr_flag], 0, 0xff, 0);
            status_render_choice();
            break;
        case MENU_STATUS_SEL_PART0: // fall through
        case MENU_STATUS_SEL_PART1: // fall through
        case MENU_STATUS_SEL_PART2: // fall through
        case MENU_STATUS_SEL_PART3: // fall through
        case MENU_STATUS_SEL_PART4: // fall through
        case MENU_STATUS_SEL_PART5: // fall through
        case MENU_STATUS_SEL_SEEN:
            break; // no action
        case MENU_STATUS_SEL_DOWNLOADED:
            break; // no action
        case MENU_STATUS_SEL_UPLOADED:
            break; // no action
        case MENU_STATUS_SEL_RADIOCAL:
            break; // no action
        default:
            break;
        }
    }
}

void controller_handle_loop() {
}

void enter_menu() {
    menu_sel = 0;
    saved_mode = qc15_mode;
}

void leave_menu() {
    qc15_mode = saved_mode;
    lcd111_clear_nodelay(LCD_TOP);
    lcd111_clear(LCD_BTM);
}

void enter_menu_status() {
    for (uint8_t i=0; i<FLAG_COUNT; i++) {
        if (&all_animations[i] == led_ring_anim_bg) {
            curr_flag = i;
            break;
        }
    }
    enter_menu();
}

void enter_menu_controller() {
    enter_menu();
}
