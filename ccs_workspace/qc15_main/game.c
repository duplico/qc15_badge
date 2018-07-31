/// The implementation of the on-badge game interaction state machine.
/**
 ** This is specific to the on-badge-only game. That is, the majority of the
 ** code-breaking part of the game goes elsewhere. This file deals with the
 ** state machine that comes in from the spreadsheets.
 **
 ** \file game.c
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <msp430.h>

#include "qc15.h"
#include "leds.h"
#include "game.h"
#include "lcd111.h"
#include "textentry.h"
#include "ipc.h"
#include "badge.h"
#include "flash_layout.h"
#include "s25fs.h"

#include "loop_signals.h"

uint8_t start_action_series(uint16_t action_id);

#define GAME_ACTION_TYPE_ANIM_TEMP 0
#define GAME_ACTION_TYPE_SET_ANIM_BG 1
#define GAME_ACTION_TYPE_STATE_TRANSITION 2
#define GAME_ACTION_TYPE_PUSH 3
#define GAME_ACTION_TYPE_POP 4
#define GAME_ACTION_TYPE_PREVIOUS 5 // This is how we'll implement "pop", to start.
#define GAME_ACTION_TYPE_CLOSE 6
#define GAME_ACTION_TYPE_NOP 7
#define GAME_ACTION_TYPE_TEXT 16
#define GAME_ACTION_TYPE_TEXT_BADGNAME 17
#define GAME_ACTION_TYPE_TEXT_USERNAME 18
#define GAME_ACTION_TYPE_TEXT_CNT 19
#define GAME_ACTION_TYPE_TEXT_END 99
#define GAME_ACTION_TYPE_OTHER 100

char game_name_buffer[QC15_BADGE_NAME_LEN];

// The following data will be loaded from the script:
led_ring_animation_t all_animations[GAME_ANIMS_LEN];

uint8_t text_cursor = 0;
game_state_t loaded_state;
game_action_t loaded_action;
uint16_t state_last_special_event = GAME_NULL;
game_state_t *current_state;
uint16_t closed_states[CLOSABLE_STATES] = {0};
uint8_t num_closed_states = 0;

char loaded_text[25];
char current_text[25] = "";
uint8_t current_text_len = 0;

uint8_t in_action_series = 0;
uint32_t game_curr_elapsed = 0;

uint8_t text_selection = 0;

const rgbcolor_t rainbow_colors[] = {
        {255, 0, 0}, // Red
        {255, 24, 0x00}, // Orange
        {128, 40, 0x00}, // Yellow
        {0, 64, 0}, // Green
        {0, 0, 196}, // Blue
        {128, 0, 128}, // Purple
};

const led_ring_animation_t anim_rainbow = {
        &rainbow_colors[0],
        6,
        5,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SPIN,
};

const rgbcolor_t lsw_colors[] = {
        {255, 255, 255},
        {255, 255, 255},
        {255, 255, 255},
        {0, 0, 0},
        {255, 255, 255},
        {255, 255, 255},
        {0, 0, 0},
        {255, 255, 255},
        {255, 255, 255},
        {255, 255, 255},
        {0, 0, 0},
        {255, 255, 255},
};

const led_ring_animation_t anim_lsw = { // "light solid white" (bright)
        &lsw_colors[0],
        12,
        2,
        HT16D_BRIGHTNESS_MAX,
        LED_ANIM_TYPE_SPIN,
};

const rgbcolor_t lwf_colors[] = { // "light white fade" (normal)
        {96, 96, 96},
        {255, 255, 255},
        {0, 0, 0},
        {96, 96, 96},
        {255, 255, 255},
        {0, 0, 0},
        {96, 96, 96},
        {255, 255, 255},
        {0, 0, 0},
};

const led_ring_animation_t anim_lwf = {
        &lwf_colors[0],
        9,
        16,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_FALL,
};

const rgbcolor_t spinblue_colors[] = { // "light white fade" (normal)
        {0, 0, 10},
        {0, 0, 32},
        {0, 0, 64},
        {0, 0, 100},
        {0, 0, 196},
        {0, 0, 255},
};

const led_ring_animation_t anim_spinblue = {
        &spinblue_colors[0],
        6,
        6,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SPIN,
};

const led_ring_animation_t anim_fallblue = {
        &spinblue_colors[0],
        6,
        6,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_FALL,
};

const led_ring_animation_t anim_solidblue = {
        &spinblue_colors[0],
        6,
        6,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SAME,
};

const rgbcolor_t whitediscovery_colors[] = { // "light white fade" (normal)
        {255, 255, 255},
        {0, 0, 0},
        {64, 64, 64},
};

const led_ring_animation_t anim_whitediscovery = {
        &whitediscovery_colors[0],
        3,
        6,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SAME,
};

const rgbcolor_t spinorange_colors[] = { // "light white fade" (normal)
        {255, 24, 0},
};

const led_ring_animation_t anim_spinorange = (led_ring_animation_t) {
        .colors = &spinorange_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
};

const rgbcolor_t solidgreen_colors[] = { // "light white fade" (normal)
        {0, 64, 0},
};

const led_ring_animation_t anim_solidgreen = (led_ring_animation_t) {
        .colors = &solidgreen_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
};

const rgbcolor_t solidyellow_colors[] = { // "light white fade" (normal)
        {128, 40, 0},
};

const led_ring_animation_t anim_solidyellow = (led_ring_animation_t) {
        .colors = &solidyellow_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
};

const led_ring_animation_t anim_spinyellow = (led_ring_animation_t) {
        .colors = &solidyellow_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
};

const led_ring_animation_t anim_fallyellow = (led_ring_animation_t) {
        .colors = &solidyellow_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_FALL,
};

const led_ring_animation_t anim_spingreen = (led_ring_animation_t) {
        .colors = &solidgreen_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
};

const rgbcolor_t solidred_colors[] = { // "light white fade" (normal)
        {255, 0, 0},
};

const led_ring_animation_t anim_spinred = (led_ring_animation_t) {
        .colors = &solidred_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
};

const led_ring_animation_t anim_solidorange = (led_ring_animation_t) {
        .colors = &spinorange_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
};

const led_ring_animation_t anim_solidred = (led_ring_animation_t) {
        .colors = &solidred_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SAME,
};

const rgbcolor_t solidwhite_colors[] = { // "light white fade" (normal)
        {255, 0, 0},
};

const led_ring_animation_t anim_spinwhite = (led_ring_animation_t) {
        .colors = &solidwhite_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
};

const rgbcolor_t solidpink_colors[] = { // "light white fade" (normal)
        {128, 0, 128},
};

const led_ring_animation_t anim_spinpink = (led_ring_animation_t) {
        .colors = &solidpink_colors[0],
        .len = 1,
        .speed = 6,
        .brightness = HT16D_BRIGHTNESS_DEFAULT,
        .type = LED_ANIM_TYPE_SPIN,
};


const rgbcolor_t pan_colors[] = {
        {0xff, 0x21, 0x8c}, // 255,33,140
        {0xff, 0xd8, 0x00}, //255,216,0
        {0xff, 0xd8, 0x00}, //255,216,0
        {0x21, 0xb1, 0xff}, //33,177,255
};

const led_ring_animation_t anim_pan = {
        &pan_colors[0],
        4,
        6,
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SPIN,
};

void load_action(game_action_t *dest, uint16_t id) {
    s25fs_read_data((uint8_t *)dest, FLASH_ADDR_GAME_ACTIONS + id*sizeof(game_action_t),
                    sizeof(game_action_t));
}

void load_state(game_state_t *dest, uint16_t id) {
    s25fs_read_data((uint8_t *)dest, FLASH_ADDR_GAME_STATES + id*sizeof(game_state_t),
                    sizeof(game_state_t));
}

void load_text(char *dest, uint16_t id) {
    s25fs_read_data((uint8_t *)dest, FLASH_ADDR_GAME_TEXT + id*25,
                    24);
    dest[25] = 0x00; // Make SURE FOR SURE it's null-terminated.
}

uint8_t state_is_closed(uint16_t state_id) {
    for (uint8_t i=0; i<num_closed_states; i++) {
        if (closed_states[i] == state_id)
            return 1;
    }
    return 0;
}

void close_state(uint16_t state_id) {
    if (!state_is_closed(state_id)) {
        closed_states[num_closed_states] = state_id;
        num_closed_states++;
    }
}

uint8_t leads_to_closed_state(uint16_t action_id) {
    game_action_t action;
    do {
        load_action(&action, action_id);
        if (action.type == GAME_ACTION_TYPE_STATE_TRANSITION &&
                state_is_closed(action.detail))
            return 1;
        action_id = action.next_action_id;
    } while (action_id != GAME_NULL);
    return 0;
}

/// Check if a special event should fire, and fire it, returning 1 if it fired.
uint8_t game_process_special() {
    uint8_t fire_special = 0;
    for (uint8_t i=0; i<current_state->other_series_len; i++) {
        // TODO: This is _really_ just a bigass OR, since every if statement
        //       has the same body.

        // TODO: We currently have a special firing OVER AND OVER AND OVER.
        //       This is a problem.

        // TODO: We should probably only consider these when we're LEAVING
        //       an action sequence (including an empty entry sequence). And
        //       most definitely NOT do the same one we just did.

        if (current_state->other_series[i].type_id == state_last_special_event)
            continue;

        if (current_state->other_series[i].type_id == SPECIAL_BADGESNEARBY0 &&
                badges_nearby==0) {
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_BADGESNEARBYSOME &&
                badges_nearby>0) {
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_NAME_FOUND &&
                s_game_checkname_success) {
            s_game_checkname_success = 0;
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_NAME_NOT_FOUND &&
                !s_game_checkname_success) {
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_SUCCESS_NEW &&
                s_gd_success == 2) { // TODO
            s_gd_success = 0;
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_SUCCESS_OLD &&
                s_gd_success == 1) { // TODO
            s_gd_success = 0;
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_FAILURE &&
                s_gd_failure) { // TODO
            s_gd_failure = 0;
            fire_special = 1;
        }
        if (fire_special) {
            fire_special = 0;
            state_last_special_event = current_state->other_series[i].type_id;
            if (start_action_series(current_state->other_series[i].result_action_id))
                return 1;
        }
    }
    return 0;
}

void game_set_state(uint16_t state_id) {
    if (state_is_closed(state_id)) {
        // State transitions to closed states have no effect.
        return;
    }
    lcd111_clear(LCD_TOP);

    last_state_id = current_state_id;
    in_action_series = 0;
    game_curr_elapsed = 0;
    text_selection = 0;

    load_state(&loaded_state, state_id);

    current_state = &loaded_state;
    current_state_id = state_id;
    state_last_special_event = GAME_NULL;

    start_action_series(current_state->entry_series_id);
}

extern uint16_t gd_starting_id;

void game_begin() {
    all_animations[0] = anim_lsw;
    all_animations[1] = anim_lwf;
    all_animations[2] = anim_spinblue;
    all_animations[3] = anim_whitediscovery;
    all_animations[4] = anim_solidblue;
    all_animations[5] = anim_spinorange;
    all_animations[6] = anim_solidgreen;
    all_animations[7] = anim_solidyellow;
    all_animations[8] = anim_spinyellow;
    all_animations[9] = anim_spingreen;
    all_animations[10] = anim_spinred;
    all_animations[11] = anim_solidorange;
    all_animations[12] = anim_solidred;
    all_animations[13] = anim_spinwhite;
    all_animations[14] = anim_spinpink;
    all_animations[15] = anim_fallblue;
    all_animations[16] = anim_fallyellow;

    // TODO: stored state
    game_set_state(STATE_ID_CRACKINGTHEFILE1);
}

uint8_t next_input_id() {
    if (!current_state->input_series_len) {
        return 0;
    }

    uint8_t ret = text_selection;
    do {
        ret += 1;
        if (ret == current_state->input_series_len+1)
            ret = 1;
    } while (leads_to_closed_state(current_state->input_series[ret-1].result_action_id));

    return ret;
}

void draw_text(uint8_t lcd_id, char *txt, uint8_t more) {
    lcd111_cursor_pos(lcd_id, 0);
    lcd111_put_char(lcd_id, 0xBB); // This is &raquo;
    lcd111_cursor_type(lcd_id, LCD111_CURSOR_NONE);
    if (strlen(txt) > 21) {
        lcd111_put_text_pad(
                lcd_id,
                txt,
                23
        );
    } else {
        lcd111_put_text_pad(
                lcd_id,
                txt,
                21
        );
        if (more) {
            lcd111_put_text_pad(lcd_id, "\x1E\x1F", 2);
        } else {
            lcd111_put_text_pad(lcd_id, " \x17", 2);
        }
    }
}

/// Render bottom screen for the current state and value of `text_selection`.
void draw_text_selection() {
    lcd111_cursor_pos(LCD_BTM, 0);
    lcd111_put_char(LCD_BTM, 0xBB); // This is &raquo;
    if (text_selection) {
        load_text(loaded_text,
                  current_state->input_series[text_selection-1].text_addr);
        draw_text(
            LCD_BTM,
            loaded_text,
            next_input_id() != text_selection
        );
    } else {
        // Haven't used an arrow key yet.
        if (current_state->input_series_len) {
            lcd111_put_text_pad(LCD_BTM, "", 21);
            if (next_input_id() != text_selection) {
                lcd111_put_text_pad(LCD_BTM, "\x1E\x1F", 2);
            } else {
                lcd111_put_text_pad(LCD_BTM, "  ", 2);
            }
            lcd111_cursor_pos(LCD_BTM, 1);
            lcd111_cursor_type(LCD_BTM, LCD111_CURSOR_BLINK);
        } else {
            lcd111_put_text_pad(LCD_BTM, "", 23);
            lcd111_cursor_type(LCD_BTM, LCD111_CURSOR_NONE);
        }

    }
}

uint8_t is_text_type(uint16_t type) {
    return (type >= GAME_ACTION_TYPE_TEXT && type < GAME_ACTION_TYPE_TEXT_END);
}

void begin_text_action() {
    current_text[24] = 0;
    current_text_len = strlen(current_text);

    lcd111_clear(1);
    lcd111_cursor_pos(1, 0);
    lcd111_cursor_type(1, LCD111_CURSOR_INVERTING);
    text_cursor = 0;

    if (is_text_type(loaded_action.type) &&
                    loaded_action.next_action_id == ACTION_NONE) {
        // Special case: show the text selection on the last frame
        //  of a text animation.
        draw_text_selection();
    }
}

// TODO: move
extern uint16_t gd_curr_id;
extern uint16_t gd_starting_id;

void do_action(game_action_t *action) {
    switch(action->type) {
    case GAME_ACTION_TYPE_ANIM_TEMP:
        // Set a temporary animation
        led_set_anim(&all_animations[action->detail], 0, action->duration, 0);
        break;
    case GAME_ACTION_TYPE_SET_ANIM_BG:
        // Set a new background animation
        if (action->detail >= GAME_ANIMS_LEN || action->detail == GAME_NULL) {
            led_set_anim_none();
        }
        led_set_anim(&all_animations[action->detail], 0, 0xFF, 0);
        break;
    case GAME_ACTION_TYPE_STATE_TRANSITION:
        // Do a state transition
        game_set_state(action->detail);
        break;
    case GAME_ACTION_TYPE_PUSH:
        // Store the current state.
        stored_state_id = current_state_id;
        break;
    case GAME_ACTION_TYPE_POP:
        // Retrieve a stored state.
        game_set_state(stored_state_id);
        break;
    case GAME_ACTION_TYPE_PREVIOUS:
        // Return to the last state.
        game_set_state(last_state_id);
        break;
    case GAME_ACTION_TYPE_CLOSE:
        close_state(current_state_id);
        break;
    case GAME_ACTION_TYPE_TEXT:
        // Display some text
        load_text(current_text, action->detail);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_BADGNAME:
        load_text(loaded_text, action->detail);
        sprintf(current_text, loaded_text, badge_conf.badge_name);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_USERNAME:
        load_text(loaded_text, action->detail);
        sprintf(current_text, loaded_text, badge_conf.person_name);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_CNT:
        load_text(loaded_text, action->detail);
        sprintf(current_text, loaded_text, badges_nearby);
        begin_text_action();
    case GAME_ACTION_TYPE_NOP:
        break; // just... do nothing.
    case GAME_ACTION_TYPE_OTHER:
        // TODO: handle
        if (action->detail == OTHER_ACTION_CUSTOMSTATEUSERNAME) {
            textentry_begin(badge_conf.person_name, 10, 1, 1);
        } else if (action->detail == OTHER_ACTION_NAMESEARCH) {
            gd_starting_id = GAME_NULL;
            gd_curr_id = GAME_NULL;
            qc15_mode = QC15_MODE_GAME_CHECKNAME;
            // IPC GET NEXT ID from ffff (any)
            while (!ipc_tx_op_buf(IPC_MSG_ID_NEXT,
                                  (uint8_t *)&gd_starting_id, 2));
            textentry_begin(game_name_buffer, 10, 0, 0);
        } else if (action->detail == OTHER_ACTION_SET_CONNECTABLE) {
            // Tell the radio module to send some connectable advertisements.
            while (!ipc_tx_byte(IPC_MSG_GD_EN));
        } else if (action->detail == OTHER_ACTION_CONNECT) {
            // Time to go into the CONNECT MODE!!!
            // The entry to this action SHOULD be guarded by a NET action
            //  that requires there to be other badges around.
            qc15_mode = QC15_MODE_GAME_CONNECT;
            gd_starting_id = GAME_NULL;
            gd_curr_id = GAME_NULL;
            while (!ipc_tx_op_buf(IPC_MSG_ID_NEXT,
                                  (uint8_t *)&gd_starting_id, 2));
            lcd111_clear(LCD_BTM);
        } else if (action->detail == OTHER_ACTION_TURN_ON_THE_LIGHTS_TO_REPRESENT_FILE_STATE) {
            badge_conf.file_lights_on = 1;
            s_turn_on_file_lights = 1;
            save_config();
        }
        break;
    }
}

/// Place the next action choice in `loaded_action`.
void select_action_choice(uint16_t first_choice_id) {
    // This function is SIDE EFFECT CITY!
    load_action(&loaded_action, first_choice_id);

    if (loaded_action.next_choice_id == ACTION_NONE) {
        // This is our last (or maybe only) choice, so we have to take it.
        return;
    }

    uint16_t random_value = rand() % loaded_action.choice_total;
    uint16_t total_choices = 0;

    do {
        total_choices += loaded_action.choice_share;
        if (loaded_action.next_choice_id == ACTION_NONE ||
                random_value < total_choices)
            return; // loaded_action fires.
        // need to get the next action:
        load_action(&loaded_action, loaded_action.next_choice_id);
    } while (1);
}

uint8_t start_action_series(uint16_t action_id) {
    if (leads_to_closed_state(action_id)) {
        return 0;
    }

    // action_id is the ID of the first action in this action series.

    // First, handle the case where this is an empty action series:
    if (action_id == ACTION_NONE) {
        in_action_series = 0;
        game_curr_elapsed = 0;
        draw_text_selection();
        return 0;
    }

    lcd111_cursor_type(LCD_BTM, LCD111_CURSOR_NONE);

    // Now, we know we're dealing with a real action series.
    // Place the appropriate action choice into `loaded_action`.
    select_action_choice(action_id);
    // We know we're in an action series, so set everything up:
    in_action_series = 1;
    game_curr_elapsed = 0;
    lcd111_set_text(0, ""); // TODO
    do_action(&loaded_action);
    return 1;
}

void game_action_sequence_tick() {
    if (is_text_type(loaded_action.type)) {
        // Check for typewritering.

        // If we haven't put all our text in yet...
        if (text_cursor < current_text_len) {
            if (game_curr_elapsed == 2) {
                game_curr_elapsed = 0;
                lcd111_put_char(LCD_TOP, current_text[text_cursor]);
                text_cursor++;
            }

            if ((text_cursor == current_text_len) || s_left) {
                // Done typing, or we were told to hurry up.
                text_cursor = current_text_len;
                lcd111_set_text(LCD_TOP, current_text);
            }

            // Now, we DON'T want this action to finish (or even for its
            //  duration to start ticking) until the typewriter is finished.
            // (if it just finished, we can wait an extra clock tick to take
            //  advantage of the auto cleanup of signals).
            return;
        }
    }

    // Duration only counts for text:
    if (loaded_action.type != GAME_ACTION_TYPE_TEXT)
        game_curr_elapsed = loaded_action.duration;

    // If we're in an action series, we block out user input.
    // Check whether the current action is completed and duration expired.
    //  If so, it's time to fire the next one.
    if (game_curr_elapsed >= loaded_action.duration ||
            ((is_text_type(loaded_action.type) && s_left))) {
        // Current action in series is over. Reset the clock.
        game_curr_elapsed = 0;
        // Time to select the next one, if there is one.
        if (loaded_action.next_action_id == ACTION_NONE) {
            // This action sequence has finished. Is there anything special
            //  going on that we need to process?
            in_action_series = 0;
            // ACTIVATE THE MENU!
            draw_text_selection();
        } else {
            // We DO have a follow-up action. Make a selection based upon
            //  the current action's next_action_id:
            select_action_choice(loaded_action.next_action_id);
            // Now run the selection.
            do_action(&loaded_action);
        }
    }
}

void next_input() {
    text_selection = next_input_id();
    draw_text_selection();
}

void prev_input() {
    if (!current_state->input_series_len) {
        return;
    }
    if (text_selection == 0) {
        text_selection = current_state->input_series_len+1;
    }
    do {
        text_selection -= 1;
        if (text_selection == 0)
            text_selection = current_state->input_series_len;
    } while (leads_to_closed_state(current_state->input_series[text_selection-1].result_action_id));
    draw_text_selection();
}

void game_process_user_in() {
    // This is a clock tick for a non-action sequence.
    // User input is unblocked, and timers are unblocked.
    // Check whether the user has changed or selected a text
    //  entry.
    if (current_state->input_series_len) {
        if (s_up) {
            next_input();
        } else if (s_down) {
            prev_input();
        } else if (s_right) {
            // Select.
            if (text_selection)
                start_action_series(current_state->input_series[text_selection-1].result_action_id);
        }
    }
}

void game_process_timers() {
    // Check whether any of our timers need to fire.
    for (uint8_t i=0; i<current_state->timer_series_len; i++) {
        // If the current elapsed time is equal to the timer duration,
        //  OR if it's a recurring timer and the elapsed time is equal to
        //  zero modulo the duration, then this timer is allowed to fire.
        // Note that this method is quite sensitive to the order in which
        //  the timers populate their array. So it's important that the
        //  instructions, above, on how to order them be followed.
        // TODO: clean this shit up (appearance-wise):
        if ((game_curr_elapsed && (
                (game_curr_elapsed == current_state->timer_series[i].duration) ||
                (current_state->timer_series[i].recurring && ((game_curr_elapsed % current_state->timer_series[i].duration) == 0))))) {
            // Time `i` should fire, unless it's closed.
            if (start_action_series(current_state->timer_series[i].result_action_id));
                break;
        }
    }
}

void game_clock_tick() {
    if (in_action_series) {
        game_action_sequence_tick();
    }

    // TODO: I'm concerned that our new way of processing special stuff,
    //       and of eliminating the durations for non-text results is going
    //       to do something funky.
    // TODO: The "AB. Someone's out there" thing isn't as punchy anymore. Why?
    if (!in_action_series) {
        game_process_special();
    }

    if (!in_action_series || (is_text_type(loaded_action.type) &&
                loaded_action.next_action_id == ACTION_NONE))
    {
        // Do buttons
        game_process_user_in();
    }

    if (!in_action_series) {
        // Do timers.
        game_process_timers();
    }

}

/// Called repeatedly from the main loop to handle steps of the game states.
void game_handle_loop() {
    if (s_clock_tick) {
        // This is the timestep signal, so increment our elapsed counter.
        game_curr_elapsed++;
        game_clock_tick();
    }
}
