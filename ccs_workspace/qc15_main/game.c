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

#include <msp430.h>

#include "qc15.h"
#include "leds.h"
#include "game.h"
#include "lcd111.h"

void start_action_series(uint16_t action_id);

#define GAME_ACTION_TYPE_TEXT 0
// TODO: TEXT_$variables
#define GAME_ACTION_TYPE_ANIM_TEMP 1
#define GAME_ACTION_TYPE_ANIM_BG 2
#define GAME_ACTION_TYPE_STATE 3
#define GAME_ACTION_TYPE_PUSH 4
#define GAME_ACTION_TYPE_POP 5
#define GAME_ACTION_TYPE_LAST 6 // This is how we'll implement "pop", to start.
#define GAME_ACTION_TYPE_CLOSE 7
#define GAME_ACTION_TYPE_OTHER 10

// The following data will be loaded from the script:
led_ring_animation_t all_animations[2];
game_state_t all_states[10];
game_action_t all_actions[10];
char all_text[][25] = {
                       "Entering initial state",
                       "Input 1",
                       "Input 2",
                       "Input 3",
                       "Input 4",
                       "This is text number 1",
                       "text number 2",
};

game_state_t loaded_state;
game_action_t loaded_action;

game_state_t *current_state;
uint16_t closed_states[MAX_CLOSED_STATES] = {0};

// TODO: persistent
uint16_t stored_state_id = 0;
uint16_t last_state_id = 0;
uint16_t current_state_id;

extern uint8_t s_down, s_up, s_left, s_right, s_clock_tick;

uint8_t in_action_series = 0;
uint16_t game_curr_elapsed = 0;

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
        "Rainbow"
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
        "Pansexual"
};

void game_set_state(uint16_t state_id) {
    last_state_id = current_state_id;
    in_action_series = 0;
    game_curr_elapsed = 0;
    text_selection = 0;

    memcpy(&loaded_state, &all_states[state_id], sizeof(game_state_t));

    current_state = &loaded_state;
    current_state_id = state_id;

    start_action_series(current_state->entry_series_id);
}

void game_begin() {
    all_animations[0] = anim_rainbow;
    all_animations[1] = anim_pan;

    all_actions[0].type =  GAME_ACTION_TYPE_ANIM_BG;
    all_actions[0].detail = 0;
    all_actions[0].duration = 0;
    all_actions[0].choice_share = 1;
    all_actions[0].choice_total = 1;
    all_actions[0].next_choice_id = ACTION_NONE;
    all_actions[0].next_action_id = 1;

    all_actions[1].type =  GAME_ACTION_TYPE_TEXT;
    all_actions[1].detail = 0;
    all_actions[1].duration = 0;
    all_actions[1].choice_share = 1;
    all_actions[1].choice_total = 1;
    all_actions[1].next_choice_id = ACTION_NONE;
    all_actions[1].next_action_id = ACTION_NONE;

    all_actions[2] = (game_action_t){
        .type = GAME_ACTION_TYPE_ANIM_TEMP,
        .detail = 1,
        .duration = 0,
        .choice_share = 1,
        .choice_total = 1,
        .next_choice_id = ACTION_NONE,
        .next_action_id = ACTION_NONE,
    };

    all_states[0].timer_series_len = 0;
    all_states[0].input_series_len = 3;
    all_states[0].input_series[0].text_addr = 1;
    all_states[0].input_series[0].result_action_id = 2;

    all_states[0].input_series[1].text_addr = 2;
    all_states[0].input_series[1].result_action_id = 2;

    all_states[0].input_series[2].text_addr = 3;
    all_states[0].input_series[2].result_action_id = 3;

    game_set_state(0);
}

void do_action(game_action_t *action) {
    switch(action->type) {
    case GAME_ACTION_TYPE_TEXT:
        // Display some text
        // TODO: Ultimately, we want a typewriter effect.
        lcd111_set_text(1, all_text[action->detail]);
        break;
    case GAME_ACTION_TYPE_ANIM_TEMP:
        // Set a temporary animation
        led_set_anim(&all_animations[action->detail], LED_ANIM_TYPE_FALL, 0, 0);
        break;
    case GAME_ACTION_TYPE_ANIM_BG:
        // Set a new background animation
        led_set_anim(&all_animations[action->detail], LED_ANIM_TYPE_FALL, 0xFF, 1);
        break;
    case GAME_ACTION_TYPE_STATE:
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
    case GAME_ACTION_TYPE_LAST:
        // Return to the last state.
        game_set_state(last_state_id);
        break;
    case GAME_ACTION_TYPE_CLOSE:
        // TODO: add this state to the list of blocked states.
        break;
    case GAME_ACTION_TYPE_OTHER:
        // TODO: handle
        break;
    }
}

/// Render bottom screen for the current state and value of `text_selection`.
void draw_text_selection() {
    lcd111_clear(0);
    if (text_selection < current_state->input_series_len) {
        lcd111_put_text(0, all_text[current_state->input_series[text_selection].text_addr], 24);
    }
}

void load_action(game_action_t *dest, uint16_t id) {
    // TODO: handle SPI flash
    memcpy(dest, &all_actions[id],
           sizeof(game_action_t));
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

void start_action_series(uint16_t action_id) {
    // action_id is the ID of the first action in this action series.

    // First, handle the case where this is an empty action series:
    if (action_id == ACTION_NONE) {
        in_action_series = 0;
        // We start this at 1, so that we can do math MOD 0 and not have
        //  every damn thing fire at the very start.
        game_curr_elapsed = 1;
        return;
    }

    // Now, we know we're dealing with a real action series.
    // Place the appropriate action choice into `loaded_action`.
    select_action_choice(action_id);
    // We know we're in an action series, so set everything up:
    in_action_series = 1;
    game_curr_elapsed = 0;
    do_action(&loaded_action);
}

/// Called repeatedly from the main loop to handle steps of the game states.
void game_handle_loop() {
    if (s_clock_tick) {
        // This is the timestep signal, so increment our elapsed counter.
        game_curr_elapsed++;
    }

    if (in_action_series) {
        // If we're in an action series, we block out user input.
        // Check whether the current action is completed and duration expired.
        //  If so, it's time to fire the next one.

        if (game_curr_elapsed >= loaded_action.duration) {
            // Current action in series is over. Reset the clock.
            game_curr_elapsed = 0;
            // Time to select the next one, if there is one.
            if (loaded_action.next_action_id == ACTION_NONE) {
                // There is no follow-up action, so this series has concluded.
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

    } else {
        // User input is unblocked, and timers are unblocked.
        // First, check whether the user has changed or selected a text
        //  entry.
        if (s_up) {
            text_selection = (text_selection + current_state->input_series_len-1) % current_state->input_series_len;
            draw_text_selection();
        } else if (s_down) {
            text_selection = (text_selection + 1) % current_state->input_series_len;
            draw_text_selection();
        } else if (s_right) {
            // Select.
            start_action_series(current_state->input_series[text_selection].result_action_id);
            return;
        }

        // Next, check whether any of our timers need to fire.
        for (uint8_t i=0; i<current_state->timer_series_len; i++) {
            // If the current elapsed time is equal to the timer duration,
            //  OR if it's a recurring timer and the elapsed time is equal to
            //  zero modulo the duration, then this timer is allowed to fire.
            // Note that this method is quite sensitive to the order in which
            //  the timers populate their array. So it's important that the
            //  instructions, above, on how to order them be followed.
            if ((game_curr_elapsed == current_state->timer_series[i].duration) ||
                    (current_state->timer_series[i].recurring && ((game_curr_elapsed % current_state->timer_series[i].duration) == 0))) {
                // Time `i` should fire.
                start_action_series(current_state->timer_series[i].result_action_id);
                break;
            }
        }
    }
}
