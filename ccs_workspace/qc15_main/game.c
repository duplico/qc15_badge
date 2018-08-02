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
#include "menu.h"

#include "led_animations.h"

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
#define GAME_ACTION_TYPE_TEXT_BADGENAME 17
#define GAME_ACTION_TYPE_TEXT_USER_NAME 18
#define GAME_ACTION_TYPE_TEXT_CNT 19
#define GAME_ACTION_TYPE_TEXT_CNCTDNAME 20
#define GAME_ACTION_TYPE_TEXT_END 99
#define GAME_ACTION_TYPE_OTHER 100

char game_name_buffer[QC15_BADGE_NAME_LEN];

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
uint32_t game_curr_action_elapsed = 0;
uint32_t game_curr_state_elapsed = 0;

uint8_t text_selection = 0;

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
        // The following is effectively a bigass OR, since every if statement
        //  has a similar (if not identical) body. But it's more readable like
        //  this.

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
                s_gd_success == 2) {
            s_gd_success = 0;
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_SUCCESS_OLD &&
                s_gd_success == 1) {
            s_gd_success = 0;
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_FAILURE &&
                s_gd_failure) {
            s_gd_failure = 0;
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_SUCCESS_DONE &&
                s_part_solved) {
            s_part_solved = 0;
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

void game_set_state(uint16_t state_id, uint8_t keep_previous) {
    if (!keep_previous && state_is_closed(state_id)) {
        // State transitions to closed states have no effect.
        return;
    }
    lcd111_clear(LCD_TOP);

    if (!keep_previous && last_state_id != game_curr_state_id)
        last_state_id = game_curr_state_id;
    in_action_series = 0;
    game_curr_action_elapsed = 0;
    game_curr_state_elapsed = 0;
    text_selection = 0;

    load_state(&loaded_state, state_id);

    current_state = &loaded_state;
    game_curr_state_id = state_id;
    state_last_special_event = GAME_NULL;

    start_action_series(current_state->entry_series_id);
}

void game_begin() {
    game_set_state(game_curr_state_id, 1);
    game_set_state(STATE_ID_CRACKINGTHEFILE1, 1); // TODO
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

void do_action(game_action_t *action) {
    switch(action->type) {
    case GAME_ACTION_TYPE_ANIM_TEMP:
        // Set a temporary animation
        if (action->detail >= GAME_ANIMS_LEN || action->detail == GAME_NULL) {
            if (led_ring_anim_bg) {
                // If we have a background animation stored, that this one
                //  was briefly superseding, then go ahead and start it
                //  up again.
                led_set_anim(led_ring_anim_bg, led_anim_type_bg,
                             0xff, led_ring_anim_pad_loops_bg);
            }
        } else {
            led_set_anim(&all_animations[action->detail], 0,
                         action->duration, 0);
        }
        break;
    case GAME_ACTION_TYPE_SET_ANIM_BG:
        // Set a new background animation
        if (action->detail >= GAME_ANIMS_LEN || action->detail == GAME_NULL) {
            led_set_anim_none();
        }
        led_set_anim(&all_animations[action->detail], 0, 0xFF, 0);
        unlock_flag(action->detail);
        break;
    case GAME_ACTION_TYPE_STATE_TRANSITION:
        // Do a state transition
        game_set_state(action->detail, 0);
        break;
    case GAME_ACTION_TYPE_PUSH:
        // Store the current state.
        stored_state_id = game_curr_state_id;
        break;
    case GAME_ACTION_TYPE_POP:
        // Retrieve a stored state.
        game_set_state(stored_state_id, 0);
        break;
    case GAME_ACTION_TYPE_PREVIOUS:
        // Return to the last state.
        game_set_state(last_state_id, 0);
        break;
    case GAME_ACTION_TYPE_CLOSE:
        close_state(game_curr_state_id);
        break;
    case GAME_ACTION_TYPE_TEXT:
        // Display some text
        load_text(current_text, action->detail);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_BADGENAME:
        load_text(loaded_text, action->detail);
        sprintf(current_text, loaded_text, badge_conf.badge_name);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_USER_NAME:
        load_text(loaded_text, action->detail);
        sprintf(current_text, loaded_text, badge_conf.person_name);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_CNT:
        load_text(loaded_text, action->detail);
        sprintf(current_text, loaded_text, badges_nearby);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_CNCTDNAME:
        load_text(loaded_text, action->detail);
        sprintf(current_text, loaded_text, badge_names[gd_curr_id]);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_NOP:
        break; // just... do nothing.
    case GAME_ACTION_TYPE_OTHER:
        if (action->detail == OTHER_ACTION_CUSTOMSTATEUSERNAME) {
            badge_conf.person_name[QC15_PERSON_NAME_LEN-1] = 0x00; // term
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
        } else if (action->detail == OTHER_ACTION_STATUS_MENU) {
            qc15_set_mode(QC15_MODE_STATUS);
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
        game_curr_action_elapsed = 0;
        draw_text_selection();
        return 0;
    }

    lcd111_cursor_type(LCD_BTM, LCD111_CURSOR_NONE);

    // Now, we know we're dealing with a real action series.
    // Place the appropriate action choice into `loaded_action`.
    select_action_choice(action_id);
    // We know we're in an action series, so set everything up:
    in_action_series = 1;
    game_curr_action_elapsed = 0;
    lcd111_set_text(0, "");
    do_action(&loaded_action);
    return 1;
}

void game_action_sequence_tick() {
    if (is_text_type(loaded_action.type)) {
        // Check for typewritering.

        // If we haven't put all our text in yet...
        if (text_cursor < current_text_len) {
            if (game_curr_action_elapsed == 2) {
                game_curr_action_elapsed = 0;
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
    } else {
        // Duration only counts for text:
        game_curr_action_elapsed = loaded_action.duration;
    }


    // If we're in an action series, we block out user input.
    // Check whether the current action is completed and duration expired.
    //  If so, it's time to fire the next one.
    if (game_curr_action_elapsed >= loaded_action.duration ||
            ((is_text_type(loaded_action.type) && s_left))) {
        // Current action in series is over. Reset the clock.
        game_curr_action_elapsed = 0;
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
        if (!game_curr_state_elapsed)
            continue; // We don't fire timers at time 0.
        // If we did, it would be *chaos* because we evaluate
        // whether a timer should fire by checking whether the
        // current state time elapsed is 0, mod the duration.
        // (which, of course, 0 always is.)
        if ((game_curr_state_elapsed == current_state->timer_series[i].duration) ||
                (current_state->timer_series[i].recurring &&
                        ((game_curr_state_elapsed % current_state->timer_series[i].duration) == 0))) {
            // Time `i` should fire, unless it's closed.
            if (start_action_series(
                    current_state->timer_series[i].result_action_id))
            {
                break;
            }
        }
    }
}

void game_clock_tick() {
    if (in_action_series) {
        game_curr_action_elapsed++;
        game_action_sequence_tick();
    }

    // Pretty much everything here has its own !in_action_series guard,
    //  because most of these things can CAUSE us to enter an action sequence,
    //  in which case we most definitely do not want to do the non-action-
    //  sequence stuff.

    if (!in_action_series)
        game_curr_state_elapsed++;

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

// Display the currently appropriate game display.
void game_render_current() {
    lcd111_clear(LCD_TOP);
    lcd111_cursor_type(LCD_TOP, LCD111_CURSOR_BLINK);
    draw_text_selection();
}

/// Called repeatedly from the main loop to handle steps of the game states.
void game_handle_loop() {
    if (s_clock_tick) {
        // This is the timestep signal, so increment our elapsed counter.
        game_clock_tick();
    }
}
