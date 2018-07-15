/// The implementation of the on-badge game interaction state machine.
/**
 ** This is specific to the on-badge-only game. That is, the majority of the
 ** code-breaking part of the game goes elsewhere. This file deals with the
 ** state machine that comes in from the spreadsheets.
 **
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#include <stdint.h>

#include <msp430.h>

#include "qc15.h"
#include "leds.h"
#include "game.h"
#include "lcd111.h"

void start_action_series(game_action_series_t* series);

// I suspect these structs will ultimately look more like the following,
//  but for now we're going to make them VERY FAT so that it's easy to
//  test and get going:

//typedef struct {
//    /// The action type ID.
//    uint8_t action_type;
//    /// Action detail number.
//    /**
//     ** In the event of an animation or change state, this is the ID of the
//     ** target. In the event of text, this signifies the address of the pointer
//     ** to the text in our text-storage system.
//     */
//    uint16_t action_detail;
//    /// The duration of the action, which may or may not be valid for this type.
//    uint8_t duration;
//} game_action_t;
//
//typedef struct {
//    /// The duration of this timer, in 1/32 of seconds.
//    uint32_t duration;
//    /// True if this timer should repeat.
//    uint8_t recurring;
//    game_action_t *result_action_series;
//} game_timer_t;
//
//typedef struct {
//    char text[25];
//    game_action_t *result_action_series;
//} game_user_in_t;
//
//typedef struct {
//    uint8_t id;
//    uint8_t entry_series_len;
//    game_action_t* entry_series;
//    /// All applicable timers for this state.
//    /**
//     ** These MUST be sorted from MOST specific to LEAST specific. That is,
//     ** any NON-recurring timers must come first, followed by recurring timers
//     ** from largest to smallest interval.
//     */
//    uint8_t timer_series_len;
//    game_timer_t* timer_series;
//    uint8_t input_series_len;
//    game_user_in_t* input_series;
//} game_state_t;

/*
 * Action types:
 * * LED animation (id, loop, duration)
 * * Display text  (text)
 * * Change state  (id)
 */

#define GAME_ACTION_TYPE_TEXT 0
#define GAME_ACTION_TYPE_ANIM_TEMP 1
#define GAME_ACTION_TYPE_ANIM_BG 2
#define GAME_ACTION_TYPE_STATE 3
#define GAME_ACTION_TYPE_OTHER 4

char all_text[][25] = {
                       "Entering initial state",
                       "Input 1",
                       "Input 2",
                       "Input 3",
                       "Input 4",
                       "This is text number 1",
                       "text number 2",
};

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

led_ring_animation_t animation_list[2];
game_state_t initial_state = {0};
game_state_t *current_state;

void game_begin() {
    animation_list[0] = anim_rainbow;
    animation_list[1] = anim_pan;

    initial_state.entry_series.len = 2;
    initial_state.entry_series.action_series[0].action_type = GAME_ACTION_TYPE_ANIM_BG;
    initial_state.entry_series.action_series[0].action_detail = 0;
    initial_state.entry_series.action_series[0].duration = 0;
    initial_state.entry_series.action_series[1].action_type = GAME_ACTION_TYPE_TEXT;
    initial_state.entry_series.action_series[1].action_detail = 0;
    initial_state.entry_series.action_series[1].duration = 0;

    initial_state.timer_series_len = 0;

    initial_state.input_series_len = 3;
    initial_state.input_series[0].text_addr = 1;
    initial_state.input_series[0].result_action_series.len = 1;
    initial_state.input_series[0].result_action_series.action_series[0].action_type = GAME_ACTION_TYPE_ANIM_TEMP;
    initial_state.input_series[0].result_action_series.action_series[0].action_detail = 1;
    initial_state.input_series[0].result_action_series.action_series[0].duration = 0;

    initial_state.input_series[1].text_addr = 2;
    initial_state.input_series[1].result_action_series.len = 1;
    initial_state.input_series[1].result_action_series.action_series[0].action_type = GAME_ACTION_TYPE_ANIM_TEMP;
    initial_state.input_series[1].result_action_series.action_series[0].action_detail = 1;
    initial_state.input_series[1].result_action_series.action_series[0].duration = 0;

    initial_state.input_series[2].text_addr = 3;
    initial_state.input_series[2].result_action_series.len = 1;
    initial_state.input_series[2].result_action_series.action_series[0].action_type = GAME_ACTION_TYPE_ANIM_TEMP;
    initial_state.input_series[2].result_action_series.action_series[0].action_detail = 1;
    initial_state.input_series[2].result_action_series.action_series[0].duration = 0;

//    memcpy(&current_state, &initial_state, sizeof(game_state_t));

    current_state = &initial_state;
    start_action_series(&(current_state->entry_series));
}

extern uint8_t s_down, s_up, s_left, s_right, s_clock_tick;

uint8_t action_series_len = 0;
uint8_t new_state = 1;


void do_action(game_action_t *action) {
    switch(action->action_type) {
    case GAME_ACTION_TYPE_TEXT:
        lcd111_set_text(1, all_text[action->action_detail]);
        break;
    case GAME_ACTION_TYPE_ANIM_TEMP:
        led_set_anim(&animation_list[action->action_detail], LED_ANIM_TYPE_FALL, 0, 0);
        break;
    case GAME_ACTION_TYPE_ANIM_BG:
        led_set_anim(&animation_list[action->action_detail], LED_ANIM_TYPE_FALL, 0xFF, 0);
        break;
    case GAME_ACTION_TYPE_STATE:
        break;
    case GAME_ACTION_TYPE_OTHER:
        // TODO: handle
        break;
    }
}

uint8_t in_action_series = 0;
game_action_series_t *curr_action_series;
uint8_t curr_action_index = 0;
uint16_t game_curr_elapsed = 0;

uint8_t text_selection = 0;

/// Render bottom screen for the current state and value of `text_selection`.
void draw_text_selection() {
    lcd111_clear(0);
    if (text_selection < current_state->input_series_len) {
        lcd111_put_text(0, all_text[current_state->input_series[text_selection].text_addr], 24);
    }
}

// TODO: Make a "NEW STATE" function

void start_action_series(game_action_series_t* series) {
    if (series->len > 0) {
        // If this series even exists:
        in_action_series = 1;
        curr_action_series = series;
        curr_action_index = 0;
        game_curr_elapsed = 0;
        do_action(&(curr_action_series->action_series[0]));
    } else {
        in_action_series = 0;
        // We start this at 1, so that we can do math MOD 0 and not have
        //  every damn thing fire at the very start.
        game_curr_elapsed = 1;
    }
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

        if (game_curr_elapsed >= curr_action_series->action_series[curr_action_index].duration) {
            // Current action in series is over.
            curr_action_index++;
            game_curr_elapsed = 0;
            if (curr_action_index >= curr_action_series->len) {
                // That was the last action in the series, so now it's time
                //  to leave the action series.
                in_action_series = 0;
                text_selection = 0;
                draw_text_selection();
            } else {
                // There are more actions in the series. We need to setup,
                //  and then fire the current index.
                do_action(&(curr_action_series->action_series[curr_action_index]));
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
            start_action_series(&(current_state->input_series[text_selection].result_action_series));
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
                start_action_series(&(current_state->timer_series[i].result_action_series));
                break;
            }
        }
    }
}
