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

void start_action_series(uint16_t action_id);

#define GAME_ACTION_TYPE_ANIM_TEMP 0
#define GAME_ACTION_TYPE_SET_ANIM_BG 1
#define GAME_ACTION_TYPE_STATE_TRANSITION 2
#define GAME_ACTION_TYPE_PUSH 3
#define GAME_ACTION_TYPE_POP 4
#define GAME_ACTION_TYPE_LAST 5 // This is how we'll implement "pop", to start.
#define GAME_ACTION_TYPE_CLOSE 6
#define GAME_ACTION_TYPE_NOP 7
#define GAME_ACTION_TYPE_TEXT 16
#define GAME_ACTION_TYPE_TEXT_BADGNAME 17
#define GAME_ACTION_TYPE_TEXT_USERNAME 18
#define GAME_ACTION_TYPE_TEXT_END 99
#define GAME_ACTION_TYPE_OTHER 100

// The following data will be loaded from the script:
led_ring_animation_t all_animations[2];

uint16_t all_actions_len = 211;
uint16_t all_text_len = 157;
uint16_t all_states_len = 157;

uint8_t all_text[][25] = {"wDFGHffdf","234vv33vdf","Cucumber","e99tftgED","Hello?","MADEUPSTATE","GO!","...","Hello(gibberish)","Gibberish","...hello?","Ok!","Sorry, what?","Got it.","English is so easy.","Now then..."," ","Who are you?","Still there?","Hellooo?","It's not a trick","question.","My name is...","Seriously?","For real?","What?","I asked you first.","You don't question ME.","You hard of hearing?","For crying out loud...","What are you?","I'm not a *thing*. Well","ok, I mean... shut up.","You going to tell me","your name or not?","Everything!","What do you do?","I am all-powerful!","I control a network...","I think.","Do you believe me?","Or something.","No matter now.","Now I talk to monkeys.","Must you waste time?","Sometimes I think you","hate me.","I need more buttons.","I've no idea what's","going on.","You there, %s?","Ah, this I know.","So who are you??","Some call me \"Mr.","Awesome\"","Some call me \"Mysterio\"","Some call me \"The Great","One\"","But you can call me...","%s here lol","So now we know each","other"," %s.","Sort of.","Wish I knew more than","that.","My status?","Status","Who me?","Damned confused.","Stuck with you.","Dark. Lost. Brilliant.","That's my status.","WHAT. DID YOU. SAY?","You're my badge.","No I am NOT.","I'm brilliant.","I'm all-powerful.","I have one fine booty.","And I need help.","From a monkey.","From you.","This SUCKS.","Do YOU have lights?","Do you have lights?","See how that sounds?","That's how you sound.","Oh wait.","Sigh...","What do you need?","Guess I'm glad you","asked.","WOHOO!","I did that.","Me. Booyah!","We have a start.","Ugh %s that's so","like you.","That's really bright.","I do a thing, and you","complain.","Fine.","Better?","Ok yes that's better.","FINALLY!","How can I help?","Stop talking about","lights for a second.","I need your help.","I'm lost.","I remember holding","something.","And I wasn't alone.","What is that so hard to","believe?","Like friends?","I'll have you know I'm","awesome.","But now I have no one.","Oh, monkey...","Hey, I'm here.","You don't count.","I mean more like me.","I've lost control.","And lost my goal.","Somehow, you must help","me.","HEY!","What's in it for me?","There's got to be a","\"shock the idot\" button","somewhere.","Try pushing buttons.","There's a good monkey!","Ok, I'll help","One problem...","I don't know what to do.","So that's our goal.","Trouble is...","I barely know this","hardware now.","...oh. My. GOD.","Any speakers?","You want speakers?","What are we playing?","Bieber?","Fraggle Rock?","A capella hair rock?","Enya's Greatest Hits?","Boot Scoot Boogie?","...shut up.","More??","More with the lights?","GAH. Can you do more","with your thumbs?","Yeah fine.","I'll try. Say if you see"};

game_action_t all_actions[] = {(game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=0, .duration=16, .next_action_id=4, .next_choice_id=1, .choice_share=1, .choice_total=4, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=1, .duration=16, .next_action_id=4, .next_choice_id=2, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=2, .duration=16, .next_action_id=4, .next_choice_id=3, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=3, .duration=16, .next_action_id=4, .next_choice_id=65535, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=0, .duration=16, .next_action_id=9, .next_choice_id=6, .choice_share=1, .choice_total=4, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=1, .duration=16, .next_action_id=9, .next_choice_id=7, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=2, .duration=16, .next_action_id=9, .next_choice_id=8, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=3, .duration=16, .next_action_id=9, .next_choice_id=65535, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=1, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=13, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=5, .duration=160, .next_action_id=13, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=0, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=7, .duration=16, .next_action_id=18, .next_choice_id=15, .choice_share=1, .choice_total=4, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=8, .duration=16, .next_action_id=18, .next_choice_id=16, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=9, .duration=16, .next_action_id=18, .next_choice_id=17, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=10, .duration=16, .next_action_id=18, .next_choice_id=65535, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=7, .duration=16, .next_action_id=23, .next_choice_id=20, .choice_share=1, .choice_total=4, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=8, .duration=16, .next_action_id=23, .next_choice_id=21, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=9, .duration=16, .next_action_id=23, .next_choice_id=22, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=10, .duration=16, .next_action_id=23, .next_choice_id=65535, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=0, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=11, .duration=16, .next_action_id=26, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=13, .duration=16, .next_action_id=27, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=14, .duration=16, .next_action_id=28, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=15, .duration=16, .next_action_id=29, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=16, .duration=16, .next_action_id=30, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=17, .duration=16, .next_action_id=31, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=2, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=18, .duration=16, .next_action_id=36, .next_choice_id=33, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=19, .duration=16, .next_action_id=36, .next_choice_id=34, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=20, .duration=16, .next_action_id=35, .next_choice_id=65535, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=21, .duration=16, .next_action_id=36, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=3, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=23, .duration=16, .next_action_id=41, .next_choice_id=39, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=24, .duration=16, .next_action_id=41, .next_choice_id=40, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=25, .duration=16, .next_action_id=41, .next_choice_id=65535, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=42, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=26, .duration=16, .next_action_id=45, .next_choice_id=43, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=27, .duration=16, .next_action_id=45, .next_choice_id=44, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=28, .duration=16, .next_action_id=45, .next_choice_id=65535, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=46, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=2, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=29, .duration=16, .next_action_id=48, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=31, .duration=16, .next_action_id=49, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=32, .duration=16, .next_action_id=50, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=33, .duration=16, .next_action_id=51, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=34, .duration=16, .next_action_id=52, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=2, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=35, .duration=16, .next_action_id=56, .next_choice_id=54, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=37, .duration=16, .next_action_id=56, .next_choice_id=55, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=38, .duration=16, .next_action_id=56, .next_choice_id=65535, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=57, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=39, .duration=16, .next_action_id=60, .next_choice_id=58, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=40, .duration=16, .next_action_id=60, .next_choice_id=59, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=41, .duration=16, .next_action_id=60, .next_choice_id=65535, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=61, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=42, .duration=16, .next_action_id=62, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=43, .duration=16, .next_action_id=64, .next_choice_id=63, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=44, .duration=16, .next_action_id=64, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=65, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=2, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=45, .duration=16, .next_action_id=67, .next_choice_id=68, .choice_share=1, .choice_total=4, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=46, .duration=16, .next_action_id=72, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=47, .duration=16, .next_action_id=72, .next_choice_id=69, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=48, .duration=16, .next_action_id=70, .next_choice_id=71, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=49, .duration=16, .next_action_id=72, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT_USERNAME, .detail=50, .duration=16, .next_action_id=72, .next_choice_id=65535, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=51, .duration=16, .next_action_id=74, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=53, .duration=16, .next_action_id=75, .next_choice_id=76, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=54, .duration=16, .next_action_id=79, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=55, .duration=16, .next_action_id=79, .next_choice_id=77, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=56, .duration=16, .next_action_id=78, .next_choice_id=65535, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=57, .duration=16, .next_action_id=79, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=80, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=58, .duration=16, .next_action_id=81, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT_BADGNAME, .detail=59, .duration=16, .next_action_id=82, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=60, .duration=16, .next_action_id=83, .next_choice_id=84, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=61, .duration=16, .next_action_id=85, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT_USERNAME, .detail=62, .duration=16, .next_action_id=85, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=86, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=63, .duration=16, .next_action_id=87, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=64, .duration=16, .next_action_id=88, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=65, .duration=16, .next_action_id=89, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=4, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=66, .duration=16, .next_action_id=92, .next_choice_id=91, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=68, .duration=16, .next_action_id=92, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=93, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=69, .duration=16, .next_action_id=96, .next_choice_id=94, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=70, .duration=16, .next_action_id=96, .next_choice_id=95, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=71, .duration=16, .next_action_id=96, .next_choice_id=65535, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=97, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=72, .duration=16, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=73, .duration=16, .next_action_id=100, .next_choice_id=99, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=75, .duration=16, .next_action_id=100, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=101, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=76, .duration=16, .next_action_id=104, .next_choice_id=102, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=77, .duration=16, .next_action_id=104, .next_choice_id=103, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=78, .duration=16, .next_action_id=104, .next_choice_id=65535, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=105, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=79, .duration=16, .next_action_id=106, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=80, .duration=16, .next_action_id=108, .next_choice_id=107, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=81, .duration=16, .next_action_id=108, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=109, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=82, .duration=16, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=83, .duration=16, .next_action_id=111, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=85, .duration=16, .next_action_id=112, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=86, .duration=16, .next_action_id=113, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=16, .duration=16, .next_action_id=114, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=87, .duration=16, .next_action_id=115, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=5, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=88, .duration=16, .next_action_id=117, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=90, .duration=16, .next_action_id=118, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=91, .duration=16, .next_action_id=119, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=7, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_SET_ANIM_BG, .detail=0, .duration=0, .next_action_id=121, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=92, .duration=16, .next_action_id=122, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=93, .duration=16, .next_action_id=123, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=94, .duration=16, .next_action_id=124, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=95, .duration=16, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT_USERNAME, .detail=96, .duration=16, .next_action_id=126, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=97, .duration=16, .next_action_id=127, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=99, .duration=16, .next_action_id=128, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=100, .duration=16, .next_action_id=129, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=101, .duration=16, .next_action_id=130, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_SET_ANIM_BG, .detail=1, .duration=0, .next_action_id=131, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=102, .duration=16, .next_action_id=132, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=103, .duration=16, .next_action_id=133, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=6, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=104, .duration=16, .next_action_id=135, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=106, .duration=16, .next_action_id=136, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=107, .duration=16, .next_action_id=137, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=108, .duration=16, .next_action_id=138, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=7, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=109, .duration=16, .next_action_id=140, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=110, .duration=16, .next_action_id=141, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=111, .duration=16, .next_action_id=142, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=112, .duration=16, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=113, .duration=16, .next_action_id=144, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=114, .duration=16, .next_action_id=145, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=116, .duration=16, .next_action_id=146, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=117, .duration=16, .next_action_id=147, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=118, .duration=16, .next_action_id=148, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=8, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=119, .duration=16, .next_action_id=150, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=121, .duration=16, .next_action_id=151, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=122, .duration=16, .next_action_id=152, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=9, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=119, .duration=16, .next_action_id=154, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=121, .duration=16, .next_action_id=155, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=122, .duration=16, .next_action_id=156, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=9, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=123, .duration=16, .next_action_id=158, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=124, .duration=16, .next_action_id=159, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=125, .duration=16, .next_action_id=160, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=126, .duration=16, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=127, .duration=16, .next_action_id=162, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=129, .duration=16, .next_action_id=163, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=130, .duration=16, .next_action_id=164, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=131, .duration=16, .next_action_id=165, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=132, .duration=16, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=133, .duration=16, .next_action_id=167, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=135, .duration=16, .next_action_id=168, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=136, .duration=16, .next_action_id=169, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_POP, .detail=0, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=137, .duration=16, .next_action_id=171, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=138, .duration=16, .next_action_id=172, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=139, .duration=16, .next_action_id=173, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=140, .duration=16, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=83, .duration=16, .next_action_id=175, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=85, .duration=16, .next_action_id=176, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=86, .duration=16, .next_action_id=177, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=16, .duration=16, .next_action_id=178, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=87, .duration=16, .next_action_id=179, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=11, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=141, .duration=16, .next_action_id=181, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=143, .duration=16, .next_action_id=182, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=144, .duration=16, .next_action_id=183, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=145, .duration=16, .next_action_id=188, .next_choice_id=184, .choice_share=1, .choice_total=5, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=146, .duration=16, .next_action_id=188, .next_choice_id=185, .choice_share=1, .choice_total=4, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=147, .duration=16, .next_action_id=188, .next_choice_id=186, .choice_share=1, .choice_total=4, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=148, .duration=16, .next_action_id=188, .next_choice_id=187, .choice_share=1, .choice_total=4, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=149, .duration=16, .next_action_id=188, .next_choice_id=65535, .choice_share=1, .choice_total=4, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_SET_ANIM_BG, .detail=0, .duration=0, .next_action_id=190, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=92, .duration=16, .next_action_id=191, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=93, .duration=16, .next_action_id=192, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=94, .duration=16, .next_action_id=193, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=95, .duration=16, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT_USERNAME, .detail=96, .duration=16, .next_action_id=195, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=97, .duration=16, .next_action_id=196, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=99, .duration=16, .next_action_id=197, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=100, .duration=16, .next_action_id=198, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=101, .duration=16, .next_action_id=199, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_SET_ANIM_BG, .detail=1, .duration=0, .next_action_id=200, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=102, .duration=16, .next_action_id=201, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=103, .duration=16, .next_action_id=202, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=150, .duration=16, .next_action_id=203, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_STATE_TRANSITION, .detail=12, .duration=0, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=151, .duration=16, .next_action_id=208, .next_choice_id=205, .choice_share=1, .choice_total=3, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=153, .duration=16, .next_action_id=206, .next_choice_id=207, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=154, .duration=16, .next_action_id=208, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=155, .duration=16, .next_action_id=208, .next_choice_id=65535, .choice_share=1, .choice_total=2, }, (game_action_t){ .type=GAME_ACTION_TYPE_NOP, .detail=0, .duration=0, .next_action_id=209, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=156, .duration=16, .next_action_id=210, .next_choice_id=65535, .choice_share=1, .choice_total=1, }, (game_action_t){ .type=GAME_ACTION_TYPE_TEXT, .detail=111, .duration=16, .next_action_id=65535, .next_choice_id=65535, .choice_share=1, .choice_total=1, }};

game_state_t all_states[] = {(game_state_t){.entry_series_id=0, .timer_series_len=1, .timer_series={(game_timer_t){.duration=64, .recurring=0, .result_action_id=5}}, .input_series_len=2, .input_series={(game_user_in_t){.text_addr=4, .result_action_id=10},(game_user_in_t){.text_addr=6, .result_action_id=11}}}, (game_state_t){.entry_series_id=14, .timer_series_len=2, .timer_series={(game_timer_t){.duration=64, .recurring=0, .result_action_id=19},(game_timer_t){.duration=256, .recurring=0, .result_action_id=24}}, .input_series_len=1, .input_series={(game_user_in_t){.text_addr=12, .result_action_id=25}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .timer_series={(game_timer_t){.duration=96, .recurring=0, .result_action_id=32}}, .input_series_len=4, .input_series={(game_user_in_t){.text_addr=22, .result_action_id=37},(game_user_in_t){.text_addr=17, .result_action_id=38},(game_user_in_t){.text_addr=30, .result_action_id=47},(game_user_in_t){.text_addr=36, .result_action_id=53}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .timer_series={(game_timer_t){.duration=480, .recurring=0, .result_action_id=66}}, .input_series_len=1, .input_series={(game_user_in_t){.text_addr=52, .result_action_id=73}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .timer_series={}, .input_series_len=4, .input_series={(game_user_in_t){.text_addr=67, .result_action_id=90},(game_user_in_t){.text_addr=74, .result_action_id=98},(game_user_in_t){.text_addr=84, .result_action_id=110},(game_user_in_t){.text_addr=89, .result_action_id=116}}}, (game_state_t){.entry_series_id=120, .timer_series_len=0, .timer_series={}, .input_series_len=1, .input_series={(game_user_in_t){.text_addr=98, .result_action_id=125}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .timer_series={}, .input_series_len=1, .input_series={(game_user_in_t){.text_addr=105, .result_action_id=134}}}, (game_state_t){.entry_series_id=139, .timer_series_len=0, .timer_series={}, .input_series_len=2, .input_series={(game_user_in_t){.text_addr=115, .result_action_id=143},(game_user_in_t){.text_addr=120, .result_action_id=149}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .timer_series={}, .input_series_len=1, .input_series={(game_user_in_t){.text_addr=120, .result_action_id=153}}}, (game_state_t){.entry_series_id=157, .timer_series_len=0, .timer_series={}, .input_series_len=2, .input_series={(game_user_in_t){.text_addr=128, .result_action_id=161},(game_user_in_t){.text_addr=134, .result_action_id=166}}}, (game_state_t){.entry_series_id=170, .timer_series_len=0, .timer_series={}, .input_series_len=2, .input_series={(game_user_in_t){.text_addr=84, .result_action_id=174},(game_user_in_t){.text_addr=142, .result_action_id=180}}}, (game_state_t){.entry_series_id=189, .timer_series_len=0, .timer_series={}, .input_series_len=1, .input_series={(game_user_in_t){.text_addr=98, .result_action_id=194}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .timer_series={}, .input_series_len=1, .input_series={(game_user_in_t){.text_addr=152, .result_action_id=204}}}, (game_state_t){.entry_series_id=12, .timer_series_len=0, .timer_series={}, .input_series_len=0, .input_series={}}};



////


uint8_t text_cursor = 0;
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

    game_set_state(0);
}

uint8_t current_text[25] = "";
uint8_t current_text_len = 0;

void begin_text_action() {
    current_text[24] = 0;
    current_text_len = strlen(current_text);

    lcd111_clear(1);
    lcd111_cursor_pos(1, 0);
    lcd111_cursor_type(1, LCD111_CURSOR_INVERTING);
    text_cursor = 0;
}

void do_action(game_action_t *action) {
    switch(action->type) {
    case GAME_ACTION_TYPE_ANIM_TEMP:
        // Set a temporary animation
        led_set_anim(&all_animations[action->detail], LED_ANIM_TYPE_FALL, 0, 0);
        break;
    case GAME_ACTION_TYPE_SET_ANIM_BG:
        // Set a new background animation
        led_set_anim(&all_animations[action->detail], LED_ANIM_TYPE_FALL, 0xFF, 1);
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
    case GAME_ACTION_TYPE_LAST:
        // Return to the last state.
        game_set_state(last_state_id);
        break;
    case GAME_ACTION_TYPE_CLOSE:
        // TODO: add this state to the list of blocked states.
        break;
    case GAME_ACTION_TYPE_TEXT:
        // Display some text
        memcpy(current_text, all_text[action->detail], 24);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_BADGNAME:
        sprintf(current_text, all_text[action->detail], badge_conf.badge_name);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_USERNAME:
        sprintf(current_text, all_text[action->detail], badge_conf.person_name);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_NOP:
        break; // just... do nothing.
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
    lcd111_set_text(0, "");
    do_action(&loaded_action);
}

uint8_t is_text_type(uint16_t type) {
    return (type >= GAME_ACTION_TYPE_TEXT && type < GAME_ACTION_TYPE_TEXT_END);
}

void game_action_sequence_tick() {
    if (is_text_type(loaded_action.type)) {
        // Check for typewritering.

        // If we haven't put all our text in yet...
        if (text_cursor < current_text_len) {
            game_curr_elapsed = 0;
            lcd111_put_char(LCD_TOP, current_text[text_cursor]);
            text_cursor++;
            if (text_cursor == current_text_len) {
                // Done typing. Disable cursor if we're done doing stuff.
                if (loaded_action.next_action_id == ACTION_NONE)
                    lcd111_cursor_type(LCD_TOP, LCD111_CURSOR_NONE);
            }
            // Now, we DON'T want this action to finish (or even for its
            //  duration to start ticking) until the typewriter is finished.
            return;
        }
    }

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
}

void game_clock_tick() {
    if (in_action_series) {
        game_action_sequence_tick();
    } else {
        // This is a clock tick for a non-action sequence.

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
            // TODO: clean this shit up (appearance-wise):
            if (game_curr_elapsed && (
                    (game_curr_elapsed == current_state->timer_series[i].duration) ||
                    (current_state->timer_series[i].recurring && ((game_curr_elapsed % current_state->timer_series[i].duration) == 0)))) {
                // Time `i` should fire.
                start_action_series(current_state->timer_series[i].result_action_id);
                break;
            }
        }
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
