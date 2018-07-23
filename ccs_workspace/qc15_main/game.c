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

#include "loop_signals.h"

void start_action_series(uint16_t action_id);

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


// The following data will be loaded from the script:
led_ring_animation_t all_animations[3];

uint16_t all_actions_len = 569;
uint16_t all_text_len = 428;
uint16_t all_states_len = 34;
uint16_t all_anims_len = 3;

uint8_t all_text[][25] = {"wDFGHffdf","234vv33vdf","Cucumber","e99tftgED","Hello?","Are you broken?"," ","Reboot.","INITIALIZING","Initialize.","....................","Completed.","I'm confused","Set state.","Welcome to customer","support, how may I help?","I'm sorry to hear that!","Badge doesn't work.","I'll be glad to help.","First we need to do a","reset.","Please unplug your","badge, wait three","seconds, then plug it","back in.","I'm sorry I have failed","to provide you adequate","assistance at this time.","Our supervisor is","available on 3rd","Tuesdays between the","hours of 3am and 3:17am","in whatever timezone you","are in.","Supervisor","Goodbye.","Have you?","I tried a reboot.","Have you really?","You think you're smarter","than me?","Do it again.","I'll wait.","I'm sorry, I cannot","discuss personal matters","with you.","You speak English!","...","Hello(gibberish)","HeGibberishllo","Gibberish","Speak English!","Learn","broke455gfr","you broken6!","Are you...","BUFFER OVERFLOW","I give up.","Speak hello","Engli-llo","Speakeng","Getting closer...","Words learn","Heword learn","Earn wordslo","Keep learning","Load English","ERR: Not in state mode","Store wordset","Rebooting","Set state:","ERR: State not found.","Language","Mode","First Run","AR112-1","ISO-3166-1.2","BCP-47","Value:","ERROR: \"You pick.\"","invalid.","You pick.","ERROR: \"English\"","English","ERROR: \"Game mode\"","Game mode","Value stored.","US","Help","You wish.","God mode","Datei nicht gefunden.","zh-DE","Neustart...","Right y'all...","en-boont-latn-021","Got it.","Language set,","awesomeness level 11.","So who are you?","Still there?","Hellooo?","It's not a trick","question.","Screw it.","That's it.","Nevermind.","We're done.","Is there another human","around?","You're killing me.","See you, human.","My name is...","%s? Seriously?","Good name for a monkey","Well %s I have no","idea what's going on.","Seriously?","Who are you?","For real?","What?","I asked you first.","You don't question ME.","You hard of hearing?","For crying out loud...","What are you?","I'm not a *thing*. Well","ok, I mean... shut up.","You going to tell me","your name or not?","Everything!","What do you do?","I am all-powerful!","I control a network...","I think.","Do you believe me?","Or something.","No matter now.","Now I talk to monkeys.","Must you waste time?","You know what?","Sometimes I think you","hate me.","I need more buttons.","I've no idea what's","going on.","You there, %s?","It's like I just woke","up.","What do you mean?","I was doing something","Holding something","Something important.","But now I don't know.","Ah, this I know.","What's your name?","Some call me \"Mr.","Awesome\"","Some call me \"Mysterio\"","Some call me \"The Great","One\"","But you can call me...","%s","So now we know each","other.","Sort of.","Wish I knew more than","that.","WHAT. DID YOU. SAY?","You're a badge.","No I am NOT.","I'm brilliant.","I'm all-powerful.","I have one fine booty.","And I need help.","From a monkey.","From you.","This SUCKS.","Right, we can do this","later.","My status?","Status","Who me?","Damned confused.","Stuck with you.","Dark, lost, and","brilliant.","That's my status.","WHAT?","You're my badge!","You don't OWN me!","If anything, I would own","YOU.","All superior beings,","raise your processing","power to 6.77 exawatts.","Yeah, thought so.","Do YOU have lights?","Do you have lights?","See how that sounds?","That's how you sound.","Oh wait.","Is it bedtime?","Go to sleep.","Ok fine.","See you later.","Sigh...","What do you need?","Guess I'm glad you","asked.","WOHOO!","I did that.","Me. Booyah!","So awesome.","Ugh %s that's so","like you.","That's really bright.","I do a thing, and you","complain.","Fine.","Better?","Ok yes that's better.","I'm lost.","I remember holding","something.","And I wasn't alone.","Is that so hard to","believe?","Like friends?","I'll have you know I'm","awesome.","But now I have no one.","Oh, monkey...","Hey, I'm here.","You don't count.","I mean more like me.","I've lost control.","And lost my goal.","Somehow, you must help","me.","I protected something.","What was your goal?","Part of it, anyway.","I assume I still have","it, whatever it is.","HEY!","What's in it for me?","There's gotta be a","\"shock the idiot\" button","somewhere.","Try pushing buttons.","Thank you!","Ok, I'll help.","One problem...","I don't know what to do.","I'm not sure.","What's your goal?","I'm sensing something","out there.","And in here.","More? Like what?","Can you do more?","Brew tea?","Make a sandwich?","I dunno, try putting me","in the freezer.","Give back rubs?","Tie your shoes?","Dolt.","Dumbass.","Seriously...","Scan? Scan what?","Scan","You, me, or... out","there?","Hey %s!","Eh, nevermind.","Hey!","Seriously!","Guess what?","I just realized!","What is it?","There's no 2!","In my code, there are 1s","and 0s but no 2s.","Sorry, can you count to","2?","Don't strain yourself.","Ok, I scanned your face.","It was funny.","Uh, ok. Scanning...","You.","Lots of awesome...","A hint of basil...","...and something else.","Really? Ok.","Me.","Nope. Nothing.","Right. Looking...","Out there.","...nothing.","Oh well.","Someone's out there.","Connecting to what?","Try connecting","There are some markings.","What should I look for?","CUSTOMSTATENAMESEARCH","Actually, yes.","Anything about me?","You're funny-lookin.","You have 11 toes.","But are nice?","But smell good?","I'm skeptical.","Tell me about...","I sure hope so.","Got it. Let's do it.","If not, ask other meat","bags with more patience.","Let's do this!","Another being, like me.","...that badge?","I can see names, now.","And we have something...","...a file. Or part of","one.","It seems there's a file.","...your goal?","Broken up. I have one","piece.","Others have pieces too.","But they're encrypted.","Together, we can crack","it.","Well I'm not alone.","...you?","I protect this file, but","it is fragmented.","And locked. No sign of","keys.","I have to brute force","this.","Dunno.","Anyone around?","Let's check.","Why, you lonely?","Hrmm...","Scanning...","Checking...","I see %d badges.","Giggity.","Let's connect.","Ok.","Connecting to network.","...oh. My. GOD.","Do you have speakers?","You want speakers?","What are we playing?","Bieber?","Fraggle Rock?","A capella hair rock?","Enya's Greatest Hits?","Boot Scoot Boogie?","CUSTOMSTATESTATUS","Status check","Busy!","A little hungry. Is that","odd?","Uff... tired.","Wondering if it's too","late to turn into a","toaster.","Why's the file locked?","More bling?","More with lights?","Not gay enough yet?","I'll try some things","Shout if you see","COLORFINDER","CRACKINGTHEFILE3","How hold do I look?","How old are you?","Younger than a CD","player.","Younger than Jason.","Younger than a million.","Younger than that","hairstyle.","Older than a proton","neutralizer.","Older than a wee babe.","Does that help?","Studying quantum","knitting.","Practicing my showtunes.","What's YOUR status?","It's...","How's the file?","Let's try this.","So no, then.","What am I seeing?","Each LED is part of my","file.","The more we unlock the","file, the more stable","the lights.","Also other stuff, go to","the badge talk.","There's a note...","\"To make closing","ceremony interesting.\"","Mean anything?","Logging off...","Connected!","We're online.","%d badges around.","%d others.","%d in range.","Nope.","Wake up!","Go away.","I'm still hurt.","Ok, fine.","Wha-?","Beep beep. Just kidding.","Ugh, I was having such a","nice dream."};

game_action_t all_actions[] = {(game_action_t){16, 0, 25, 65535, 1, 1, 4}, (game_action_t){16, 1, 26, 65535, 2, 1, 4}, (game_action_t){16, 2, 24, 65535, 3, 1, 4}, (game_action_t){16, 3, 25, 65535, 65535, 1, 4}, (game_action_t){16, 0, 25, 65535, 5, 1, 4}, (game_action_t){16, 1, 26, 65535, 6, 1, 4}, (game_action_t){16, 2, 24, 65535, 7, 1, 4}, (game_action_t){16, 3, 25, 65535, 65535, 1, 4}, (game_action_t){2, 2, 0, 65535, 65535, 1, 1}, (game_action_t){2, 3, 0, 65535, 65535, 1, 1}, (game_action_t){16, 6, 17, 11, 65535, 1, 1}, (game_action_t){16, 6, 17, 12, 65535, 1, 1}, (game_action_t){16, 6, 17, 13, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 15, 65535, 1, 1}, (game_action_t){16, 10, 36, 16, 65535, 1, 1}, (game_action_t){16, 11, 26, 17, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 1, 0, 65535, 65535, 1, 1}, (game_action_t){2, 6, 0, 65535, 65535, 1, 1}, (game_action_t){16, 14, 35, 21, 65535, 1, 1}, (game_action_t){16, 15, 40, 65535, 65535, 1, 1}, (game_action_t){16, 16, 39, 23, 65535, 1, 1}, (game_action_t){16, 18, 37, 24, 65535, 1, 1}, (game_action_t){16, 19, 37, 25, 65535, 1, 1}, (game_action_t){16, 20, 22, 26, 65535, 1, 1}, (game_action_t){16, 21, 34, 27, 65535, 1, 1}, (game_action_t){16, 22, 33, 28, 65535, 1, 1}, (game_action_t){16, 23, 37, 29, 65535, 1, 1}, (game_action_t){16, 24, 24, 65535, 65535, 1, 1}, (game_action_t){16, 25, 39, 31, 65535, 1, 1}, (game_action_t){16, 26, 39, 32, 65535, 1, 1}, (game_action_t){16, 27, 40, 33, 65535, 1, 1}, (game_action_t){16, 28, 33, 34, 65535, 1, 1}, (game_action_t){16, 29, 32, 35, 65535, 1, 1}, (game_action_t){16, 30, 36, 36, 65535, 1, 1}, (game_action_t){16, 31, 39, 37, 65535, 1, 1}, (game_action_t){16, 32, 40, 38, 65535, 1, 1}, (game_action_t){16, 33, 23, 39, 65535, 1, 1}, (game_action_t){16, 35, 24, 40, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 36, 25, 42, 65535, 1, 1}, (game_action_t){16, 38, 32, 43, 65535, 1, 1}, (game_action_t){16, 39, 40, 44, 65535, 1, 1}, (game_action_t){16, 40, 24, 45, 65535, 1, 1}, (game_action_t){16, 41, 28, 46, 65535, 1, 1}, (game_action_t){16, 42, 26, 65535, 65535, 1, 1}, (game_action_t){16, 43, 35, 48, 65535, 1, 1}, (game_action_t){16, 44, 40, 49, 65535, 1, 1}, (game_action_t){16, 45, 25, 65535, 65535, 1, 1}, (game_action_t){16, 47, 19, 65535, 51, 1, 4}, (game_action_t){16, 48, 32, 65535, 52, 1, 4}, (game_action_t){16, 49, 30, 65535, 53, 1, 4}, (game_action_t){16, 50, 25, 65535, 65535, 1, 4}, (game_action_t){16, 47, 19, 65535, 55, 1, 4}, (game_action_t){16, 48, 32, 65535, 56, 1, 4}, (game_action_t){16, 49, 30, 65535, 57, 1, 4}, (game_action_t){16, 50, 25, 65535, 65535, 1, 4}, (game_action_t){16, 8, 28, 59, 65535, 1, 1}, (game_action_t){16, 10, 36, 60, 65535, 1, 1}, (game_action_t){16, 11, 26, 61, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 4, 0, 65535, 65535, 1, 1}, (game_action_t){2, 5, 0, 65535, 65535, 1, 1}, (game_action_t){16, 53, 27, 65535, 65, 1, 3}, (game_action_t){16, 54, 28, 65535, 66, 1, 3}, (game_action_t){16, 55, 26, 65535, 65535, 1, 3}, (game_action_t){16, 56, 31, 68, 65535, 1, 1}, (game_action_t){16, 8, 28, 69, 65535, 1, 1}, (game_action_t){16, 10, 36, 70, 65535, 1, 1}, (game_action_t){16, 11, 26, 71, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 58, 27, 65535, 73, 1, 4}, (game_action_t){16, 59, 25, 65535, 74, 1, 4}, (game_action_t){16, 60, 24, 65535, 75, 1, 4}, (game_action_t){16, 48, 32, 65535, 65535, 1, 4}, (game_action_t){16, 58, 27, 65535, 77, 1, 4}, (game_action_t){16, 59, 25, 65535, 78, 1, 4}, (game_action_t){16, 60, 24, 65535, 79, 1, 4}, (game_action_t){16, 48, 32, 65535, 65535, 1, 4}, (game_action_t){16, 56, 31, 81, 65535, 1, 1}, (game_action_t){16, 8, 28, 82, 65535, 1, 1}, (game_action_t){16, 10, 36, 83, 65535, 1, 1}, (game_action_t){16, 11, 26, 84, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 6, 17, 86, 65535, 1, 1}, (game_action_t){16, 6, 17, 87, 65535, 1, 1}, (game_action_t){16, 6, 17, 88, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 62, 27, 65535, 90, 1, 4}, (game_action_t){16, 63, 28, 65535, 91, 1, 4}, (game_action_t){16, 50, 25, 65535, 92, 1, 4}, (game_action_t){16, 64, 28, 65535, 65535, 1, 4}, (game_action_t){16, 62, 27, 65535, 94, 1, 4}, (game_action_t){16, 63, 28, 65535, 95, 1, 4}, (game_action_t){16, 50, 25, 65535, 96, 1, 4}, (game_action_t){16, 64, 28, 65535, 65535, 1, 4}, (game_action_t){16, 56, 31, 98, 65535, 1, 1}, (game_action_t){16, 8, 28, 99, 65535, 1, 1}, (game_action_t){16, 10, 36, 100, 65535, 1, 1}, (game_action_t){16, 11, 26, 101, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 56, 31, 103, 65535, 1, 1}, (game_action_t){16, 8, 28, 104, 65535, 1, 1}, (game_action_t){16, 10, 36, 105, 65535, 1, 1}, (game_action_t){16, 11, 26, 106, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 67, 38, 108, 65535, 1, 1}, (game_action_t){16, 69, 25, 109, 65535, 1, 1}, (game_action_t){16, 6, 17, 110, 65535, 1, 1}, (game_action_t){16, 6, 17, 111, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 113, 65535, 1, 1}, (game_action_t){16, 10, 36, 114, 65535, 1, 1}, (game_action_t){16, 11, 26, 115, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 70, 26, 65535, 65535, 1, 1}, (game_action_t){16, 71, 37, 118, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 71, 37, 120, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 71, 37, 122, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 71, 37, 124, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 7, 0, 65535, 65535, 1, 1}, (game_action_t){2, 8, 0, 65535, 65535, 1, 1}, (game_action_t){16, 78, 22, 65535, 65535, 1, 1}, (game_action_t){16, 79, 34, 129, 65535, 1, 1}, (game_action_t){16, 80, 24, 130, 65535, 1, 1}, (game_action_t){16, 69, 25, 131, 65535, 1, 1}, (game_action_t){16, 6, 17, 132, 65535, 1, 1}, (game_action_t){16, 6, 17, 133, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 82, 32, 135, 65535, 1, 1}, (game_action_t){16, 80, 24, 136, 65535, 1, 1}, (game_action_t){16, 69, 25, 137, 65535, 1, 1}, (game_action_t){16, 6, 17, 138, 65535, 1, 1}, (game_action_t){16, 6, 17, 139, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 84, 34, 141, 65535, 1, 1}, (game_action_t){16, 80, 24, 142, 65535, 1, 1}, (game_action_t){16, 69, 25, 143, 65535, 1, 1}, (game_action_t){16, 6, 17, 144, 65535, 1, 1}, (game_action_t){16, 6, 17, 145, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 86, 29, 147, 65535, 1, 1}, (game_action_t){16, 69, 25, 148, 65535, 1, 1}, (game_action_t){16, 6, 17, 149, 65535, 1, 1}, (game_action_t){16, 6, 17, 150, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 78, 22, 65535, 65535, 1, 1}, (game_action_t){16, 82, 32, 153, 65535, 1, 1}, (game_action_t){16, 80, 24, 154, 65535, 1, 1}, (game_action_t){16, 69, 25, 155, 65535, 1, 1}, (game_action_t){16, 6, 17, 156, 65535, 1, 1}, (game_action_t){16, 6, 17, 157, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 89, 25, 160, 65535, 1, 1}, (game_action_t){16, 69, 25, 161, 65535, 1, 1}, (game_action_t){16, 6, 17, 162, 65535, 1, 1}, (game_action_t){16, 6, 17, 163, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 91, 37, 165, 65535, 1, 1}, (game_action_t){16, 93, 27, 166, 65535, 1, 1}, (game_action_t){16, 6, 17, 167, 65535, 1, 1}, (game_action_t){16, 6, 17, 168, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 94, 30, 170, 65535, 1, 1}, (game_action_t){16, 96, 23, 171, 65535, 1, 1}, (game_action_t){16, 97, 29, 172, 65535, 1, 1}, (game_action_t){16, 98, 37, 173, 65535, 1, 1}, (game_action_t){16, 99, 31, 174, 65535, 1, 1}, (game_action_t){2, 9, 0, 65535, 65535, 1, 1}, (game_action_t){16, 100, 28, 65535, 176, 1, 3}, (game_action_t){16, 101, 24, 65535, 177, 1, 3}, (game_action_t){16, 102, 32, 178, 65535, 1, 3}, (game_action_t){16, 103, 25, 65535, 65535, 1, 1}, (game_action_t){16, 104, 25, 182, 180, 1, 3}, (game_action_t){16, 105, 26, 182, 181, 1, 3}, (game_action_t){16, 106, 26, 182, 65535, 1, 3}, (game_action_t){16, 107, 27, 187, 183, 1, 4}, (game_action_t){16, 108, 38, 184, 185, 1, 4}, (game_action_t){16, 109, 23, 187, 65535, 1, 1}, (game_action_t){16, 110, 34, 187, 186, 1, 4}, (game_action_t){16, 111, 31, 187, 65535, 1, 4}, (game_action_t){2, 28, 0, 65535, 65535, 1, 1}, (game_action_t){100, 0, 0, 189, 65535, 1, 1}, (game_action_t){18, 113, 37, 191, 190, 1, 2}, (game_action_t){16, 114, 38, 191, 65535, 1, 2}, (game_action_t){18, 115, 40, 192, 65535, 1, 1}, (game_action_t){16, 116, 37, 193, 65535, 1, 1}, (game_action_t){2, 10, 0, 65535, 65535, 1, 1}, (game_action_t){16, 117, 26, 197, 195, 1, 3}, (game_action_t){16, 119, 25, 197, 196, 1, 3}, (game_action_t){16, 120, 21, 197, 65535, 1, 3}, (game_action_t){16, 121, 34, 200, 198, 1, 3}, (game_action_t){16, 122, 38, 200, 199, 1, 3}, (game_action_t){16, 123, 36, 200, 65535, 1, 3}, (game_action_t){2, 9, 0, 65535, 65535, 1, 1}, (game_action_t){16, 124, 38, 202, 65535, 1, 1}, (game_action_t){16, 126, 39, 203, 65535, 1, 1}, (game_action_t){16, 127, 38, 204, 65535, 1, 1}, (game_action_t){16, 128, 36, 205, 65535, 1, 1}, (game_action_t){16, 129, 33, 206, 65535, 1, 1}, (game_action_t){2, 9, 0, 65535, 65535, 1, 1}, (game_action_t){16, 130, 27, 210, 208, 1, 3}, (game_action_t){16, 132, 34, 210, 209, 1, 3}, (game_action_t){16, 133, 38, 210, 65535, 1, 3}, (game_action_t){16, 134, 24, 213, 211, 1, 3}, (game_action_t){16, 135, 34, 213, 212, 1, 3}, (game_action_t){16, 136, 29, 213, 65535, 1, 3}, (game_action_t){16, 137, 30, 214, 65535, 1, 1}, (game_action_t){16, 138, 38, 216, 215, 1, 2}, (game_action_t){16, 139, 36, 216, 65535, 1, 2}, (game_action_t){2, 9, 0, 65535, 65535, 1, 1}, (game_action_t){16, 104, 25, 220, 218, 1, 3}, (game_action_t){16, 105, 26, 220, 219, 1, 3}, (game_action_t){16, 140, 30, 220, 65535, 1, 3}, (game_action_t){16, 107, 27, 225, 221, 1, 4}, (game_action_t){16, 108, 38, 222, 223, 1, 4}, (game_action_t){16, 109, 23, 225, 65535, 1, 1}, (game_action_t){16, 110, 34, 225, 224, 1, 4}, (game_action_t){16, 111, 31, 225, 65535, 1, 4}, (game_action_t){2, 28, 0, 65535, 65535, 1, 1}, (game_action_t){16, 141, 37, 227, 228, 1, 4}, (game_action_t){16, 142, 24, 65535, 65535, 1, 1}, (game_action_t){16, 143, 36, 65535, 229, 1, 4}, (game_action_t){16, 144, 35, 230, 231, 1, 4}, (game_action_t){16, 145, 25, 65535, 65535, 1, 1}, (game_action_t){18, 146, 37, 65535, 65535, 1, 4}, (game_action_t){16, 147, 37, 233, 65535, 1, 1}, (game_action_t){16, 148, 19, 234, 65535, 1, 1}, (game_action_t){16, 150, 37, 235, 65535, 1, 1}, (game_action_t){16, 151, 33, 236, 65535, 1, 1}, (game_action_t){16, 152, 36, 237, 65535, 1, 1}, (game_action_t){16, 153, 37, 65535, 65535, 1, 1}, (game_action_t){16, 154, 32, 239, 65535, 1, 1}, (game_action_t){16, 156, 33, 240, 241, 1, 3}, (game_action_t){16, 157, 24, 244, 65535, 1, 1}, (game_action_t){16, 158, 39, 244, 242, 1, 3}, (game_action_t){16, 159, 39, 243, 65535, 1, 3}, (game_action_t){16, 160, 20, 244, 65535, 1, 1}, (game_action_t){16, 161, 38, 245, 65535, 1, 1}, (game_action_t){17, 162, 25, 246, 65535, 1, 1}, (game_action_t){16, 163, 35, 247, 65535, 1, 1}, (game_action_t){16, 164, 22, 248, 65535, 1, 1}, (game_action_t){16, 165, 24, 249, 65535, 1, 1}, (game_action_t){16, 166, 37, 250, 65535, 1, 1}, (game_action_t){16, 167, 21, 251, 65535, 1, 1}, (game_action_t){2, 11, 0, 65535, 65535, 1, 1}, (game_action_t){16, 168, 35, 254, 253, 1, 2}, (game_action_t){16, 170, 28, 254, 65535, 1, 2}, (game_action_t){16, 171, 30, 257, 255, 1, 3}, (game_action_t){16, 172, 33, 257, 256, 1, 3}, (game_action_t){16, 173, 38, 257, 65535, 1, 3}, (game_action_t){16, 174, 32, 258, 65535, 1, 1}, (game_action_t){16, 175, 30, 260, 259, 1, 2}, (game_action_t){16, 176, 25, 260, 65535, 1, 2}, (game_action_t){16, 177, 27, 65535, 65535, 1, 1}, (game_action_t){16, 141, 37, 262, 263, 1, 4}, (game_action_t){16, 142, 24, 65535, 65535, 1, 1}, (game_action_t){16, 143, 36, 65535, 264, 1, 4}, (game_action_t){16, 144, 35, 265, 266, 1, 4}, (game_action_t){16, 145, 25, 65535, 65535, 1, 1}, (game_action_t){18, 146, 37, 65535, 65535, 1, 4}, (game_action_t){16, 178, 37, 268, 65535, 1, 1}, (game_action_t){16, 179, 22, 269, 65535, 1, 1}, (game_action_t){2, 29, 0, 65535, 65535, 1, 1}, (game_action_t){16, 180, 26, 272, 271, 1, 2}, (game_action_t){16, 182, 23, 272, 65535, 1, 2}, (game_action_t){16, 183, 32, 276, 273, 1, 3}, (game_action_t){16, 184, 31, 276, 274, 1, 3}, (game_action_t){16, 185, 31, 275, 65535, 1, 3}, (game_action_t){16, 186, 26, 276, 65535, 1, 1}, (game_action_t){16, 187, 33, 65535, 65535, 1, 1}, (game_action_t){16, 188, 21, 278, 65535, 1, 1}, (game_action_t){16, 190, 33, 279, 65535, 1, 1}, (game_action_t){16, 191, 40, 280, 65535, 1, 1}, (game_action_t){16, 192, 20, 281, 65535, 1, 1}, (game_action_t){16, 193, 36, 282, 65535, 1, 1}, (game_action_t){16, 194, 37, 283, 65535, 1, 1}, (game_action_t){16, 195, 39, 284, 65535, 1, 1}, (game_action_t){16, 196, 33, 65535, 65535, 1, 1}, (game_action_t){16, 197, 35, 286, 65535, 1, 1}, (game_action_t){16, 199, 36, 287, 65535, 1, 1}, (game_action_t){16, 200, 37, 288, 65535, 1, 1}, (game_action_t){16, 6, 17, 289, 65535, 1, 1}, (game_action_t){16, 201, 24, 290, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 202, 30, 293, 292, 1, 2}, (game_action_t){16, 204, 24, 293, 65535, 1, 2}, (game_action_t){16, 205, 30, 294, 65535, 1, 1}, (game_action_t){2, 29, 0, 65535, 65535, 1, 1}, (game_action_t){16, 206, 23, 296, 65535, 1, 1}, (game_action_t){16, 208, 34, 297, 65535, 1, 1}, (game_action_t){16, 209, 22, 298, 65535, 1, 1}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){1, 0, 0, 300, 65535, 1, 1}, (game_action_t){16, 210, 22, 301, 65535, 1, 1}, (game_action_t){16, 211, 27, 302, 65535, 1, 1}, (game_action_t){16, 212, 27, 303, 65535, 1, 1}, (game_action_t){16, 213, 27, 65535, 65535, 1, 1}, (game_action_t){18, 214, 39, 305, 65535, 1, 1}, (game_action_t){16, 215, 25, 306, 65535, 1, 1}, (game_action_t){16, 217, 37, 307, 65535, 1, 1}, (game_action_t){16, 218, 25, 308, 65535, 1, 1}, (game_action_t){16, 219, 21, 309, 65535, 1, 1}, (game_action_t){1, 1, 0, 310, 65535, 1, 1}, (game_action_t){16, 220, 23, 311, 65535, 1, 1}, (game_action_t){16, 6, 17, 312, 65535, 1, 1}, (game_action_t){16, 221, 37, 313, 65535, 1, 1}, (game_action_t){6, 0, 0, 314, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 222, 25, 316, 65535, 1, 1}, (game_action_t){16, 223, 34, 317, 65535, 1, 1}, (game_action_t){16, 224, 26, 318, 65535, 1, 1}, (game_action_t){16, 225, 35, 65535, 65535, 1, 1}, (game_action_t){16, 226, 34, 320, 65535, 1, 1}, (game_action_t){16, 227, 24, 321, 65535, 1, 1}, (game_action_t){16, 229, 38, 322, 65535, 1, 1}, (game_action_t){16, 230, 24, 323, 65535, 1, 1}, (game_action_t){16, 231, 38, 324, 65535, 1, 1}, (game_action_t){2, 14, 0, 65535, 65535, 1, 1}, (game_action_t){16, 232, 29, 326, 65535, 1, 1}, (game_action_t){16, 234, 32, 327, 65535, 1, 1}, (game_action_t){16, 235, 36, 328, 65535, 1, 1}, (game_action_t){2, 15, 0, 65535, 65535, 1, 1}, (game_action_t){16, 232, 29, 330, 65535, 1, 1}, (game_action_t){16, 234, 32, 331, 65535, 1, 1}, (game_action_t){16, 235, 36, 332, 65535, 1, 1}, (game_action_t){2, 15, 0, 65535, 65535, 1, 1}, (game_action_t){16, 236, 34, 334, 65535, 1, 1}, (game_action_t){16, 237, 33, 335, 65535, 1, 1}, (game_action_t){16, 238, 38, 336, 65535, 1, 1}, (game_action_t){16, 239, 19, 65535, 65535, 1, 1}, (game_action_t){16, 240, 38, 338, 65535, 1, 1}, (game_action_t){16, 242, 35, 339, 65535, 1, 1}, (game_action_t){16, 243, 37, 340, 65535, 1, 1}, (game_action_t){16, 244, 35, 65535, 65535, 1, 1}, (game_action_t){16, 245, 20, 342, 65535, 1, 1}, (game_action_t){16, 247, 34, 343, 65535, 1, 1}, (game_action_t){16, 248, 40, 344, 65535, 1, 1}, (game_action_t){16, 249, 26, 345, 65535, 1, 1}, (game_action_t){16, 250, 36, 65535, 65535, 1, 1}, (game_action_t){16, 251, 26, 347, 65535, 1, 1}, (game_action_t){16, 253, 30, 348, 65535, 1, 1}, (game_action_t){16, 254, 40, 349, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){2, 17, 0, 65535, 351, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){16, 197, 35, 353, 65535, 1, 1}, (game_action_t){16, 199, 36, 354, 65535, 1, 1}, (game_action_t){16, 200, 37, 355, 65535, 1, 1}, (game_action_t){16, 6, 17, 356, 65535, 1, 1}, (game_action_t){16, 201, 24, 357, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 255, 29, 359, 65535, 1, 1}, (game_action_t){16, 257, 37, 360, 65535, 1, 1}, (game_action_t){16, 258, 26, 361, 65535, 1, 1}, (game_action_t){16, 259, 28, 65535, 65535, 1, 1}, (game_action_t){16, 260, 32, 363, 65535, 1, 1}, (game_action_t){16, 262, 25, 369, 364, 1, 5}, (game_action_t){16, 263, 32, 369, 365, 1, 5}, (game_action_t){16, 264, 39, 366, 367, 1, 5}, (game_action_t){16, 265, 31, 369, 65535, 1, 1}, (game_action_t){16, 266, 31, 369, 368, 1, 5}, (game_action_t){16, 267, 31, 369, 65535, 1, 5}, (game_action_t){16, 268, 21, 65535, 370, 1, 3}, (game_action_t){16, 269, 24, 65535, 371, 1, 3}, (game_action_t){16, 270, 28, 65535, 65535, 1, 3}, (game_action_t){16, 271, 32, 373, 65535, 1, 1}, (game_action_t){16, 273, 34, 374, 65535, 1, 1}, (game_action_t){16, 274, 22, 375, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){1, 2, 0, 377, 65535, 1, 1}, (game_action_t){18, 275, 30, 65535, 65535, 1, 1}, (game_action_t){16, 276, 30, 379, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 277, 20, 65535, 381, 1, 4}, (game_action_t){18, 275, 30, 65535, 382, 1, 4}, (game_action_t){16, 278, 26, 65535, 383, 1, 4}, (game_action_t){16, 279, 27, 65535, 65535, 1, 4}, (game_action_t){16, 280, 32, 385, 65535, 1, 1}, (game_action_t){16, 282, 29, 386, 65535, 1, 1}, (game_action_t){16, 283, 40, 387, 65535, 1, 1}, (game_action_t){16, 284, 33, 388, 65535, 1, 1}, (game_action_t){16, 285, 39, 389, 65535, 1, 1}, (game_action_t){16, 286, 18, 390, 65535, 1, 1}, (game_action_t){16, 287, 38, 391, 65535, 1, 1}, (game_action_t){6, 0, 0, 392, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 288, 40, 394, 65535, 1, 1}, (game_action_t){16, 289, 29, 395, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){16, 290, 35, 397, 65535, 1, 1}, (game_action_t){16, 292, 34, 399, 398, 1, 2}, (game_action_t){16, 293, 34, 399, 65535, 1, 2}, (game_action_t){16, 6, 17, 400, 65535, 1, 1}, (game_action_t){16, 294, 38, 401, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){16, 295, 27, 403, 65535, 1, 1}, (game_action_t){16, 6, 17, 404, 65535, 1, 1}, (game_action_t){16, 297, 30, 405, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){16, 298, 33, 407, 65535, 1, 1}, (game_action_t){16, 6, 17, 408, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 300, 27, 410, 65535, 1, 1}, (game_action_t){16, 301, 24, 411, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){18, 162, 25, 413, 65535, 1, 1}, (game_action_t){16, 302, 36, 414, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 303, 35, 416, 65535, 1, 1}, (game_action_t){16, 305, 40, 417, 65535, 1, 1}, (game_action_t){16, 306, 39, 418, 65535, 1, 1}, (game_action_t){2, 30, 0, 65535, 65535, 1, 1}, (game_action_t){16, 307, 160, 420, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 308, 30, 422, 65535, 1, 1}, (game_action_t){16, 310, 36, 424, 423, 1, 2}, (game_action_t){16, 311, 33, 424, 65535, 1, 2}, (game_action_t){16, 312, 29, 426, 425, 1, 2}, (game_action_t){16, 313, 31, 426, 65535, 1, 2}, (game_action_t){16, 314, 30, 65535, 65535, 1, 1}, (game_action_t){2, 22, 0, 65535, 65535, 1, 1}, (game_action_t){16, 316, 31, 429, 65535, 1, 1}, (game_action_t){16, 318, 38, 430, 65535, 1, 1}, (game_action_t){16, 319, 40, 431, 65535, 1, 1}, (game_action_t){16, 320, 30, 432, 65535, 1, 1}, (game_action_t){2, 23, 0, 65535, 65535, 1, 1}, (game_action_t){16, 321, 39, 434, 65535, 1, 1}, (game_action_t){16, 323, 37, 435, 65535, 1, 1}, (game_action_t){16, 324, 40, 436, 65535, 1, 1}, (game_action_t){16, 325, 37, 437, 65535, 1, 1}, (game_action_t){16, 326, 20, 438, 65535, 1, 1}, (game_action_t){2, 21, 0, 65535, 65535, 1, 1}, (game_action_t){16, 327, 40, 440, 65535, 1, 1}, (game_action_t){16, 329, 37, 441, 65535, 1, 1}, (game_action_t){16, 330, 22, 442, 65535, 1, 1}, (game_action_t){16, 331, 39, 443, 65535, 1, 1}, (game_action_t){16, 332, 38, 444, 65535, 1, 1}, (game_action_t){16, 333, 38, 445, 65535, 1, 1}, (game_action_t){16, 334, 19, 446, 65535, 1, 1}, (game_action_t){2, 21, 0, 65535, 65535, 1, 1}, (game_action_t){16, 335, 35, 448, 65535, 1, 1}, (game_action_t){16, 337, 40, 449, 65535, 1, 1}, (game_action_t){16, 338, 33, 450, 65535, 1, 1}, (game_action_t){16, 339, 38, 451, 65535, 1, 1}, (game_action_t){16, 340, 21, 452, 65535, 1, 1}, (game_action_t){16, 341, 37, 453, 65535, 1, 1}, (game_action_t){16, 342, 21, 454, 65535, 1, 1}, (game_action_t){2, 21, 0, 65535, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 343, 22, 460, 457, 1, 4}, (game_action_t){16, 345, 28, 460, 458, 1, 4}, (game_action_t){16, 346, 32, 460, 459, 1, 4}, (game_action_t){16, 347, 23, 460, 65535, 1, 4}, (game_action_t){16, 348, 27, 462, 461, 1, 2}, (game_action_t){16, 349, 27, 462, 65535, 1, 2}, (game_action_t){19, 350, 34, 65535, 65535, 1, 1}, (game_action_t){16, 351, 24, 465, 464, 1, 2}, (game_action_t){16, 353, 19, 465, 65535, 1, 2}, (game_action_t){16, 354, 38, 466, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 355, 31, 468, 65535, 1, 1}, (game_action_t){16, 357, 34, 469, 65535, 1, 1}, (game_action_t){16, 358, 36, 470, 65535, 1, 1}, (game_action_t){16, 359, 23, 65535, 471, 1, 5}, (game_action_t){16, 360, 29, 65535, 472, 1, 5}, (game_action_t){16, 361, 36, 65535, 473, 1, 5}, (game_action_t){16, 362, 37, 65535, 474, 1, 5}, (game_action_t){16, 363, 34, 65535, 65535, 1, 5}, (game_action_t){2, 31, 0, 65535, 478, 9, 13}, (game_action_t){16, 364, 160, 477, 65535, 1, 1}, (game_action_t){2, 23, 0, 65535, 65535, 1, 1}, (game_action_t){16, 366, 21, 65535, 479, 1, 13}, (game_action_t){16, 367, 40, 480, 481, 1, 13}, (game_action_t){16, 368, 20, 65535, 65535, 1, 1}, (game_action_t){16, 369, 29, 65535, 482, 1, 13}, (game_action_t){16, 370, 37, 483, 65535, 1, 13}, (game_action_t){16, 371, 35, 484, 65535, 1, 1}, (game_action_t){16, 372, 24, 65535, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 374, 27, 488, 487, 1, 2}, (game_action_t){16, 376, 35, 488, 65535, 1, 2}, (game_action_t){16, 377, 36, 489, 65535, 1, 1}, (game_action_t){16, 378, 32, 490, 65535, 1, 1}, (game_action_t){16, 224, 26, 491, 65535, 1, 1}, (game_action_t){2, 32, 0, 65535, 65535, 1, 1}, (game_action_t){16, 379, 160, 493, 65535, 1, 1}, (game_action_t){2, 23, 0, 65535, 65535, 1, 1}, (game_action_t){2, 33, 0, 65535, 65535, 1, 1}, (game_action_t){16, 380, 160, 496, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 343, 22, 501, 498, 1, 4}, (game_action_t){16, 345, 28, 501, 499, 1, 4}, (game_action_t){16, 346, 32, 501, 500, 1, 4}, (game_action_t){16, 347, 23, 501, 65535, 1, 4}, (game_action_t){16, 348, 27, 503, 502, 1, 2}, (game_action_t){16, 349, 27, 503, 65535, 1, 2}, (game_action_t){19, 350, 34, 65535, 65535, 1, 1}, (game_action_t){16, 351, 24, 506, 505, 1, 2}, (game_action_t){16, 353, 19, 506, 65535, 1, 2}, (game_action_t){16, 354, 38, 507, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 381, 35, 509, 65535, 1, 1}, (game_action_t){16, 383, 33, 510, 511, 1, 4}, (game_action_t){16, 384, 23, 515, 65535, 1, 1}, (game_action_t){16, 385, 35, 515, 512, 1, 4}, (game_action_t){16, 386, 39, 515, 513, 1, 4}, (game_action_t){16, 387, 33, 514, 65535, 1, 4}, (game_action_t){16, 388, 26, 515, 65535, 1, 1}, (game_action_t){16, 389, 35, 516, 517, 1, 2}, (game_action_t){16, 390, 28, 518, 65535, 1, 1}, (game_action_t){16, 391, 38, 518, 65535, 1, 2}, (game_action_t){16, 392, 31, 65535, 65535, 1, 1}, (game_action_t){2, 31, 0, 65535, 520, 9, 12}, (game_action_t){16, 393, 32, 521, 522, 1, 12}, (game_action_t){16, 394, 25, 65535, 65535, 1, 1}, (game_action_t){16, 395, 40, 65535, 523, 1, 12}, (game_action_t){16, 396, 35, 65535, 65535, 1, 12}, (game_action_t){16, 374, 27, 526, 525, 1, 2}, (game_action_t){16, 376, 35, 526, 65535, 1, 2}, (game_action_t){16, 377, 36, 527, 65535, 1, 1}, (game_action_t){16, 378, 32, 528, 65535, 1, 1}, (game_action_t){16, 224, 26, 529, 65535, 1, 1}, (game_action_t){2, 32, 0, 65535, 65535, 1, 1}, (game_action_t){16, 397, 23, 531, 65535, 1, 1}, (game_action_t){16, 140, 30, 532, 65535, 1, 1}, (game_action_t){16, 399, 31, 533, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){100, 1, 0, 535, 65535, 1, 1}, (game_action_t){16, 392, 31, 65535, 65535, 1, 1}, (game_action_t){16, 400, 28, 537, 65535, 1, 1}, (game_action_t){16, 402, 38, 538, 65535, 1, 1}, (game_action_t){16, 403, 21, 539, 65535, 1, 1}, (game_action_t){16, 404, 38, 540, 65535, 1, 1}, (game_action_t){16, 405, 37, 541, 65535, 1, 1}, (game_action_t){16, 406, 27, 542, 65535, 1, 1}, (game_action_t){16, 407, 39, 543, 65535, 1, 1}, (game_action_t){16, 408, 31, 544, 65535, 1, 1}, (game_action_t){6, 0, 0, 545, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 409, 33, 547, 65535, 1, 1}, (game_action_t){16, 410, 32, 548, 65535, 1, 1}, (game_action_t){16, 411, 38, 549, 65535, 1, 1}, (game_action_t){16, 412, 30, 550, 65535, 1, 1}, (game_action_t){6, 0, 0, 551, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 413, 30, 553, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 414, 26, 556, 555, 1, 2}, (game_action_t){16, 415, 29, 556, 65535, 1, 2}, (game_action_t){19, 416, 35, 65535, 557, 1, 3}, (game_action_t){19, 417, 28, 65535, 558, 1, 3}, (game_action_t){19, 418, 30, 65535, 65535, 1, 3}, (game_action_t){16, 419, 21, 562, 560, 1, 3}, (game_action_t){16, 421, 24, 562, 561, 1, 3}, (game_action_t){16, 422, 31, 562, 65535, 1, 3}, (game_action_t){16, 6, 96, 65535, 65535, 1, 1}, (game_action_t){16, 423, 25, 65535, 65535, 1, 1}, (game_action_t){16, 424, 21, 568, 565, 1, 3}, (game_action_t){16, 425, 40, 568, 566, 1, 3}, (game_action_t){16, 426, 40, 567, 65535, 1, 3}, (game_action_t){16, 427, 27, 568, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}};

game_state_t all_states[] = {(game_state_t){.entry_series_id=0, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=4}}, .input_series={(game_user_in_t){.text_addr=4, .result_action_id=8},(game_user_in_t){.text_addr=5, .result_action_id=9},(game_user_in_t){.text_addr=7, .result_action_id=10},(game_user_in_t){.text_addr=9, .result_action_id=14},(game_user_in_t){.text_addr=12, .result_action_id=18},(game_user_in_t){.text_addr=13, .result_action_id=19}}, .other_series={}}, (game_state_t){.entry_series_id=20, .timer_series_len=0, .input_series_len=4, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=17, .result_action_id=22},(game_user_in_t){.text_addr=34, .result_action_id=30},(game_user_in_t){.text_addr=37, .result_action_id=41},(game_user_in_t){.text_addr=46, .result_action_id=47}}, .other_series={}}, (game_state_t){.entry_series_id=50, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=54}}, .input_series={(game_user_in_t){.text_addr=9, .result_action_id=58},(game_user_in_t){.text_addr=51, .result_action_id=62},(game_user_in_t){.text_addr=52, .result_action_id=63}}, .other_series={}}, (game_state_t){.entry_series_id=64, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=57, .result_action_id=67}}, .other_series={}}, (game_state_t){.entry_series_id=72, .timer_series_len=1, .input_series_len=2, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=76}}, .input_series={(game_user_in_t){.text_addr=61, .result_action_id=80},(game_user_in_t){.text_addr=7, .result_action_id=85}}, .other_series={}}, (game_state_t){.entry_series_id=89, .timer_series_len=1, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=93}}, .input_series={(game_user_in_t){.text_addr=65, .result_action_id=97},(game_user_in_t){.text_addr=66, .result_action_id=102},(game_user_in_t){.text_addr=68, .result_action_id=107},(game_user_in_t){.text_addr=9, .result_action_id=112}}, .other_series={}}, (game_state_t){.entry_series_id=116, .timer_series_len=0, .input_series_len=6, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=72, .result_action_id=117},(game_user_in_t){.text_addr=73, .result_action_id=119},(game_user_in_t){.text_addr=74, .result_action_id=121},(game_user_in_t){.text_addr=75, .result_action_id=123},(game_user_in_t){.text_addr=76, .result_action_id=125},(game_user_in_t){.text_addr=77, .result_action_id=126}}, .other_series={}}, (game_state_t){.entry_series_id=127, .timer_series_len=0, .input_series_len=4, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=81, .result_action_id=128},(game_user_in_t){.text_addr=83, .result_action_id=134},(game_user_in_t){.text_addr=85, .result_action_id=140},(game_user_in_t){.text_addr=87, .result_action_id=146}}, .other_series={}}, (game_state_t){.entry_series_id=151, .timer_series_len=0, .input_series_len=5, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=83, .result_action_id=152},(game_user_in_t){.text_addr=88, .result_action_id=158},(game_user_in_t){.text_addr=90, .result_action_id=159},(game_user_in_t){.text_addr=92, .result_action_id=164},(game_user_in_t){.text_addr=95, .result_action_id=169}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=175},(game_timer_t){.duration=1152, .recurring=0, .result_action_id=179}}, .input_series={(game_user_in_t){.text_addr=112, .result_action_id=188},(game_user_in_t){.text_addr=118, .result_action_id=194},(game_user_in_t){.text_addr=125, .result_action_id=201},(game_user_in_t){.text_addr=131, .result_action_id=207}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=640, .recurring=0, .result_action_id=217},(game_timer_t){.duration=480, .recurring=0, .result_action_id=226}}, .input_series={(game_user_in_t){.text_addr=149, .result_action_id=232},(game_user_in_t){.text_addr=155, .result_action_id=238},(game_user_in_t){.text_addr=169, .result_action_id=252}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=480, .recurring=0, .result_action_id=261},(game_timer_t){.duration=38400, .recurring=0, .result_action_id=267}}, .input_series={(game_user_in_t){.text_addr=181, .result_action_id=270},(game_user_in_t){.text_addr=189, .result_action_id=277},(game_user_in_t){.text_addr=198, .result_action_id=285},(game_user_in_t){.text_addr=203, .result_action_id=291},(game_user_in_t){.text_addr=207, .result_action_id=295}}, .other_series={}}, (game_state_t){.entry_series_id=299, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=216, .result_action_id=304}}, .other_series={}}, (game_state_t){.entry_series_id=315, .timer_series_len=0, .input_series_len=2, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=228, .result_action_id=319},(game_user_in_t){.text_addr=233, .result_action_id=325}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=233, .result_action_id=329}}, .other_series={}}, (game_state_t){.entry_series_id=333, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=241, .result_action_id=337},(game_user_in_t){.text_addr=246, .result_action_id=341},(game_user_in_t){.text_addr=252, .result_action_id=346}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=90240, .recurring=0, .result_action_id=350}}, .input_series={(game_user_in_t){.text_addr=198, .result_action_id=352},(game_user_in_t){.text_addr=256, .result_action_id=358},(game_user_in_t){.text_addr=261, .result_action_id=362},(game_user_in_t){.text_addr=272, .result_action_id=372}}, .other_series={}}, (game_state_t){.entry_series_id=376, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=960, .recurring=0, .result_action_id=378},(game_timer_t){.duration=64, .recurring=0, .result_action_id=380}}, .input_series={(game_user_in_t){.text_addr=281, .result_action_id=384}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=393}}, .input_series={(game_user_in_t){.text_addr=291, .result_action_id=396},(game_user_in_t){.text_addr=296, .result_action_id=402},(game_user_in_t){.text_addr=299, .result_action_id=406}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=0, .other_series_len=2, .timer_series={}, .input_series={}, .other_series={(game_other_in_t){.type_id=0, .result_action_id=409},(game_other_in_t){.type_id=1, .result_action_id=412}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=304, .result_action_id=415}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=309, .result_action_id=421},(game_user_in_t){.text_addr=315, .result_action_id=427},(game_user_in_t){.text_addr=317, .result_action_id=428}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=322, .result_action_id=433},(game_user_in_t){.text_addr=328, .result_action_id=439},(game_user_in_t){.text_addr=336, .result_action_id=447}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=455}}, .input_series={(game_user_in_t){.text_addr=344, .result_action_id=456},(game_user_in_t){.text_addr=352, .result_action_id=463},(game_user_in_t){.text_addr=356, .result_action_id=467},(game_user_in_t){.text_addr=365, .result_action_id=475},(game_user_in_t){.text_addr=373, .result_action_id=485},(game_user_in_t){.text_addr=375, .result_action_id=486}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=494}}, .input_series={(game_user_in_t){.text_addr=344, .result_action_id=497},(game_user_in_t){.text_addr=352, .result_action_id=504},(game_user_in_t){.text_addr=382, .result_action_id=508},(game_user_in_t){.text_addr=365, .result_action_id=519},(game_user_in_t){.text_addr=375, .result_action_id=524},(game_user_in_t){.text_addr=398, .result_action_id=530}}, .other_series={}}, (game_state_t){.entry_series_id=534, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=401, .result_action_id=536}}, .other_series={}}, (game_state_t){.entry_series_id=546, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=554, .timer_series_len=1, .input_series_len=0, .other_series_len=0, .timer_series={(game_timer_t){.duration=320, .recurring=0, .result_action_id=552}}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=563}}, .input_series={(game_user_in_t){.text_addr=420, .result_action_id=559}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=420, .result_action_id=564}}, .other_series={}}, (game_state_t){.entry_series_id=419, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=476, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=492, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=495, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}};

#define SPECIAL_BADGESNEARBY0 0
#define SPECIAL_BADGESNEARBYMORETHAN1 1
#define OTHER_ACTION_CUSTOMSTATEUSERNAME 0
#define OTHER_ACTION_TURN_ON_THE_LIGHTS_TO_REPRESENT_FILE_STATE 1
#define CLOSABLE_STATES 4


////


uint8_t text_cursor = 0;
game_state_t loaded_state;
game_action_t loaded_action;
game_state_t *current_state;
uint16_t closed_states[CLOSABLE_STATES] = {0};
uint8_t num_closed_states = 0;

uint8_t current_text[25] = "";
uint8_t current_text_len = 0;


// TODO: persistent
uint16_t stored_state_id = 0;
uint16_t last_state_id = 0;
uint16_t current_state_id;

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

void load_action(game_action_t *dest, uint16_t id) {
    // TODO: handle SPI flash
    memcpy(dest, &all_actions[id],
           sizeof(game_action_t));
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

void game_set_state(uint16_t state_id) {
    if (state_is_closed(state_id)) {
        // State transitions to closed states have no effect.
        return;
    }

    last_state_id = current_state_id;
    in_action_series = 0;
    game_curr_elapsed = 0;
    text_selection = 0;

    memcpy(&loaded_state, &all_states[state_id], sizeof(game_state_t));

    current_state = &loaded_state;
    current_state_id = state_id;

    // First, check for special stuff...
    for (uint8_t i=0; i<current_state->other_series_len; i++) {
        // Handle the specialized inputs that replace the ENTRY event:
        if (leads_to_closed_state(current_state->other_series[i].result_action_id))
            continue; // Mask events that lead to closed states.

        if (current_state->other_series[i].type_id == SPECIAL_BADGESNEARBY0 &&
                badges_nearby==0) {
            start_action_series(current_state->other_series[i].result_action_id);
            return;
        }
        if (current_state->other_series[i].type_id == SPECIAL_BADGESNEARBYMORETHAN1 &&
                badges_nearby>0) {
            start_action_series(current_state->other_series[i].result_action_id);
            return;
        }
    }

    if (!leads_to_closed_state(current_state->entry_series_id)) {
        start_action_series(current_state->entry_series_id);
    }
}

void game_begin() {
    all_animations[0] = anim_rainbow;
    all_animations[1] = anim_pan;
    all_animations[2] = anim_rainbow;

    game_set_state(9);
}

/// Render bottom screen for the current state and value of `text_selection`.
void draw_text_selection() {
    lcd111_cursor_pos(LCD_BTM, 0);
    lcd111_put_char(LCD_BTM, 0xBB); // This is &raquo;
    lcd111_put_text_pad(
            LCD_BTM,
            all_text[current_state->input_series[text_selection].text_addr],
            23
    );
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
        led_set_anim(&all_animations[action->detail], LED_ANIM_TYPE_FALL, 0, 0);
        break;
    case GAME_ACTION_TYPE_SET_ANIM_BG:
        // Set a new background animation
        if (action->detail >= all_anims_len || action->detail == GAME_NULL) {
            led_set_anim_none();
        }
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
    case GAME_ACTION_TYPE_PREVIOUS:
        // Return to the last state.
        game_set_state(last_state_id);
        break;
    case GAME_ACTION_TYPE_CLOSE:
        close_state(current_state_id);
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
    case GAME_ACTION_TYPE_TEXT_CNT:
        sprintf(current_text, all_text[action->detail], badges_nearby);
        begin_text_action();

    case GAME_ACTION_TYPE_NOP:
        break; // just... do nothing.
    case GAME_ACTION_TYPE_OTHER:
        // TODO: handle
        if (action->detail == OTHER_ACTION_CUSTOMSTATEUSERNAME)
            textentry_begin(badge_conf.person_name, 10, 1);
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

void start_action_series(uint16_t action_id) {
    // action_id is the ID of the first action in this action series.

    // First, handle the case where this is an empty action series:
    if (action_id == ACTION_NONE) {
        in_action_series = 0;
        game_curr_elapsed = 0;
        draw_text_selection();
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

void game_action_sequence_tick() {
    if (is_text_type(loaded_action.type)) {
        // Check for typewritering.

        // If we haven't put all our text in yet...
        if (text_cursor < current_text_len) {
            if (game_curr_elapsed == 2) {
                game_curr_elapsed = 0;
                lcd111_put_char(LCD_TOP, current_text[text_cursor]);
                text_cursor++;
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
    }

    // If we're in an action series, we block out user input.
    // Check whether the current action is completed and duration expired.
    //  If so, it's time to fire the next one.
    if (game_curr_elapsed >= loaded_action.duration ||
            ((is_text_type(loaded_action.type) && s_left))) {
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

void next_input() {
    do {
        text_selection = (text_selection + current_state->input_series_len-1) % current_state->input_series_len;
    } while (leads_to_closed_state(current_state->input_series[text_selection].result_action_id));
    draw_text_selection();
}

void prev_input() {
    do {
        text_selection = (text_selection + 1) % current_state->input_series_len;
    } while (leads_to_closed_state(current_state->input_series[text_selection].result_action_id));
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
            // TODO: only do this if there's a valid input.
            start_action_series(current_state->input_series[text_selection].result_action_id);
            return;
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
                (current_state->timer_series[i].recurring && ((game_curr_elapsed % current_state->timer_series[i].duration) == 0)))) &&
                !leads_to_closed_state(current_state->timer_series[i].result_action_id)) {
            // Time `i` should fire.
            start_action_series(current_state->timer_series[i].result_action_id);
            break;
        }
    }
}

void game_clock_tick() {
    if (in_action_series) {
        game_action_sequence_tick();
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

    // TODO: The following (currently) can only happen in a clock tick:
    if (in_action_series) {
        // User input is blocked.
        if (is_text_type(loaded_action.type) &&
                loaded_action.next_action_id == ACTION_NONE) {
            // SPECIAL CASE: unblock input if we're just text, and this is
            //  its last frame.
        } else {
            return;
        }
    }


}
