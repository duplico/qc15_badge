/// Declarations for the on-badge game interaction state machine.
/**
 ** \file game.h
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#ifndef GAME_H_
#define GAME_H_

#include "state_definitions.h"

#define ACTION_NONE 0xFFFF
#define ANIM_NONE   0xFFFF
#define GAME_NULL   0xFFFF

/// A node in an action series and/or choice set.
typedef struct {
    /// The action type ID.
    uint16_t type;
    /// Action detail number.
    /**
     ** In the event of an animation or change state, this is the ID of the
     ** target. In the event of text, this signifies the address of the pointer
     ** to the text in our text-storage system.
     */
    uint16_t detail;
    /// The duration of the action, which may or may not be valid for this type.
    uint16_t duration;
    /// The ID of the next action to fire after this one, or `ACTION_NONE`.
    uint16_t next_action_id;
    /// The ID of the next possible choice in this choice set, or `ACTION_NONE`.
    uint16_t next_choice_id;
    /// The share of the likelihood of this event firing.
    uint16_t choice_share;
    /// The total choice shares (denominator) of all choices in this choice set.
    uint16_t choice_total;
} game_action_t;

typedef struct {
    /// The duration of this timer, in 1/32 of seconds.
    uint32_t duration;
    /// True if this timer should repeat.
    uint8_t recurring;
    uint16_t result_action_id;
} game_timer_t;

typedef struct {
    uint16_t text_addr;
    uint16_t result_action_id;
} game_user_in_t;

typedef struct {
    uint16_t type_id;
    uint16_t result_action_id;
} game_other_in_t;

typedef struct {
    uint16_t entry_series_id;
    /// All applicable timers for this state.
    /**
     ** These MUST be sorted from MOST specific to LEAST specific. That is,
     ** any NON-recurring timers must come first, followed by recurring timers
     ** from largest to smallest interval.
     */
    uint8_t timer_series_len;
    uint8_t input_series_len;
    uint8_t other_series_len;

    game_timer_t timer_series[MAX_TIMERS];
    game_user_in_t input_series[MAX_INPUTS];
    game_other_in_t other_series[MAX_OTHERS];
} game_state_t;

extern char game_name_buffer[QC15_BADGE_NAME_LEN];
extern uint8_t s_game_checkname_success;
extern uint8_t s_turn_on_file_lights;

void game_begin();
void game_handle_loop();
void game_render_current();

#endif /* GAME_H_ */
