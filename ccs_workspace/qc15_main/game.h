/*
 * game.h
 *
 *  Created on: Jul 15, 2018
 *      Author: george
 */

#ifndef GAME_H_
#define GAME_H_
typedef struct {
    /// The action type ID.
    uint8_t action_type;
    /// Action detail number.
    /**
     ** In the event of an animation or change state, this is the ID of the
     ** target. In the event of text, this signifies the address of the pointer
     ** to the text in our text-storage system.
     */
    uint16_t action_detail;
    /// The duration of the action, which may or may not be valid for this type.
    uint8_t duration;
} game_action_t;

typedef struct {
    uint8_t len;
    game_action_t action_series[5];
} game_action_series_t;

typedef struct {
    /// The duration of this timer, in 1/32 of seconds.
    uint32_t duration;
    /// True if this timer should repeat.
    uint8_t recurring;
    game_action_series_t result_action_series;
} game_timer_t;

typedef struct {
    uint16_t text_addr;
    game_action_series_t result_action_series;
} game_user_in_t;

typedef struct {
    uint8_t id;
    game_action_series_t entry_series;
    /// All applicable timers for this state.
    /**
     ** These MUST be sorted from MOST specific to LEAST specific. That is,
     ** any NON-recurring timers must come first, followed by recurring timers
     ** from largest to smallest interval.
     */
    uint8_t timer_series_len;
    game_timer_t timer_series[5];
    uint8_t input_series_len;
    game_user_in_t input_series[5];
} game_state_t;

void game_begin();
void game_handle_loop();

#endif /* GAME_H_ */
