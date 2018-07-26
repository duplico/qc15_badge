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

uint16_t all_actions_len = 659;
uint16_t all_text_len = 435;
uint16_t all_states_len = 40;
uint16_t all_anims_len = 3;

// lightsSolidWhite, lightsWhiteFader, whiteDiscovery
uint8_t all_text[][25] = {"wDFGHffdf","234vv33vdf","Cucumber","e99tftgED","","Hello?","Are you broken?","Reboot.","INITIALIZING","Initialize.","....................","Completed.","I'm confused","Set state.","Welcome to customer","support, how may I help?","I'm sorry to hear that!","Badge doesn't work.","I'll be glad to help.","First we need to do a","reset.","Please unplug your","badge, wait three","seconds, then plug it","back in.","I'm sorry I have failed","to provide you adequate","assistance at this time.","Our supervisor is","available on 3rd","Tuesdays between the","hours of 3am and 3:17am","in whatever timezone you","are in.","Supervisor","Goodbye.","Have you?","I tried a reboot.","Have you really?","You think you're smarter","than me?","Do it again.","I'll wait.","I'm sorry, I cannot","discuss personal matters","with you.","You speak English!","...","Hello(gibberish)","HeGibberishllo","Gibberish","Speak English!","Learn","broke455gfr","you broken6!","Are you...","BUFFER OVERFLOW","I give up.","Rebooting","Speak hello","Engli-llo","Speakeng","Getting closer...","Words learn","Heword learn","Earn wordslo","Keep learning","Load English","ERR: Not in state mode","Store wordset","Set state:","ERR: State not found.","Language","Mode","First Run","AR112-1","ISO-3166-1.2","BCP-47","Value:","ERR: \"You pick.\"","invalid.","You pick.","ERR: \"English\" invalid.","English","ERR: \"Game mode\"","Game mode","Value stored.","US","Help","You wish.","God mode","Datei nicht gefunden.","zh-DE","Neustart...","en-boont-latn-021","Hello! I am -","....","Waitaminute.","What's going on?","Sysclock just says","12:00","Ohh, crap.","I've lost main memory.","CRAP!","This is not good.","And who the hell are","you?","Still there?","Hellooo?","It's not a trick","question.","Screw it.","That's it.","Nevermind.","We're done.","Waste my time...","You're killing me.","See you, human.","My name is...","%s?","Good name for a monkey.","Well %s I have no","idea what's going on.","Seriously?","Who are you?","For real?","What?","I asked you first.","You don't question ME.","You hard of hearing?","For crying out loud...","What are you?","I'm not a *thing*. Well","ok, I mean... shut up.","You going to tell me","your name or not?","Everything!","What do you do?","I am all-powerful!","I control a file...","I think.","Do you believe me?","Or something.","No matter now.","You know what?","Helloo?","Waste my time.","Sigh...","I need more buttons.","So frustrated.","You there, %s?","It's like I just woke","up.","What do you mean?","I was doing something","Holding something","Something important.","But now I don't know.","Ah, this I know.","What's your name?","Some call me \"Mr.","Awesome\"","Some call me \"Mysterio\"","Some call me \"The Great","One\"","But you can call me...","%s","Now we know each other.","Sort of.","Wish I knew more than","that.","WHAT. DID YOU. SAY?","You're a badge.","No I am NOT.","I'm brilliant.","I'm all-powerful.","I have one fine booty.","And I need help.","From a monkey.","From you.","This SUCKS.","I feel stuck.","Do you smell that?","Reboots are a killer.","Ok, later then.","My status?","Status","Who me?","You're curious, eh.","Damned confused.","Stuck with you.","Dark, lost, and","brilliant.","Dumbstruck by a reboot.","That's my status.","WHAT?","You're my badge!","You don't OWN me!","If anything, I would own","YOU.","All superior beings,","raise your processing","power to 6.77 exawatts.","Yeah, thought so.","Do YOU have lights?","Do you have lights?","See how that sounds?","That's how you sound.","Oh wait.","What do you need?","Guess I'm glad you","asked.","WOHOO!","I did that.","Me. Booyah!","So awesome.","Ugh %s that's so","like you.","That's really bright.","I do a thing, and you","complain.","Fine.","Better?","Ok yes that's better.","I'm lost.","I remember holding","something.","And I wasn't alone.","Is that so hard to","believe?","Like friends?","I'll have you know I'm","awesome.","But now I have no one.","Oh, monkey...","Hey, I'm here.","You don't count.","I mean more like me.","I've lost control.","And lost my goal.","Somehow, you must help","me.","I protected something.","What was your goal?","Part of it, anyway.","I assume I still have","it, whatever it is.","HEY!","What's in it for me?","There's gotta be a","\"shock the idiot\" button","somewhere.","Try pushing buttons.","Thank you!","Ok, I'll help.","One problem...","I don't know what to do.","Humm...","I am awesome...","Soooo awesome...","The awesome is meee.....","I'm not sure.","What's your goal?","I'm sensing something","out there.","And in here.","More? Like what?","Can you do more?","Brew tea?","Make a sandwich?","I dunno, try putting me","in the freezer.","Give back rubs?","Tie your shoes?","Dolt.","Dumbass.","Seriously...","Scan? Scan what?","Scan","You, me, or... out","there?","Hey %s!","Eh, nevermind.","Hey!","Seriously!","Guess what?","I just realized!","What is it?","There's no 2!","In my code, there are 1s","and 0s but no 2s.","Can you count to 2?","Don't strain yourself.","Ok, I scanned your face.","It was funny.","Uh, ok. Scanning...","You.","Lots of awesome...","A hint of basil...","...and something else.","Really? Ok.","Me.","Nope. Nothing.","Right. Looking...","Out there.","...nothing.","Oh well.","%s.","Someone's out there.","Connecting to what?","Try connecting","There are some markings.","What should I look for?","CUSTOMSTATENAMESEARCH","Actually, yes.","Anything about me?","You're funny-lookin.","You have 11 toes.","But are nice?","But smell good?","I'm skeptical.","Tell me about...","I sure hope so.","Got it. Let's do it.","If not, ask other meat","bags with more patience.","Let's do this!","Another being, like me.","...that badge?","I can see names, now.","And we have something...","...a file. Or part of","one.","It seems there's a file.","...your goal?","Broken up. I have one","piece.","Others have pieces too.","But they're encrypted.","Together, we can crack","it.","Well I'm not alone.","...you?","I protect this file, but","it is fragmented.","And locked. No sign of","keys.","I have to brute force","this.","Dunno.","Anyone around?","Let's check.","Why, you lonely?","Hrmm...","Scanning...","Checking...","I see %d badges.","Giggity.","Let's connect.","Ok.","Connecting to network.","CUSTOMSTATESTATUS","Status check","Busy!","A little hungry. Is that","odd?","Uff... tired.","Wondering if it's too","late to turn into a","toaster.","Why's the file locked?","Do you have speakers?","More bling?","More with lights?","Not gay enough yet?","I'll try some things","Shout if you see","COLORFINDER","How old are you?","Studying quantum","knitting.","Practicing my showtunes.","What's YOUR status?","It's...","How's the file?","Let's try this.","WHYSTHEFILEENCRYPTED","Why's it encrypted?","DOYOUHAVEALLTHEFILE","Do you have it all?","Bugger off.","Does that help?","So no, then.","What am I seeing?","Each LED is part of my","file.","The more we unlock the","file, the more stable","the lights.","Also other stuff, go to","the badge talk.","There's a note...","\"To make closing","ceremony interesting.\"","Mean anything?","Logging off...","Connected!","We're online.","%d badges around.","%d others.","%d in range.","Nope.","Wake up!","Go away.","I'm still hurt.","Ok, fine.","Now you be nicer!","Where were we?","Wha-?","Beep beep. Just kidding.","Ugh, I was having such a","nice dream.","...oh. My. GOD.","You want speakers?","What are we playing?","Bieber?","Fraggle Rock?","A capella hair metal?","I only play showtunes.","First off, rude.","Second, still rude.","I dunno...","About 15K of your years.","But younger than those","who made me."};

game_action_t all_actions[] = {(game_action_t){16, 0, 25, 4, 1, 1, 4}, (game_action_t){16, 1, 26, 4, 2, 1, 4}, (game_action_t){16, 2, 24, 4, 3, 1, 4}, (game_action_t){16, 3, 25, 4, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 0, 25, 9, 6, 1, 4}, (game_action_t){16, 1, 26, 9, 7, 1, 4}, (game_action_t){16, 2, 24, 9, 8, 1, 4}, (game_action_t){16, 3, 25, 9, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 65535, 1, 1}, (game_action_t){2, 3, 0, 65535, 65535, 1, 1}, (game_action_t){16, 4, 17, 13, 65535, 1, 1}, (game_action_t){16, 4, 17, 14, 65535, 1, 1}, (game_action_t){16, 4, 17, 15, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 17, 65535, 1, 1}, (game_action_t){16, 10, 36, 18, 65535, 1, 1}, (game_action_t){16, 11, 26, 19, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 1, 0, 65535, 65535, 1, 1}, (game_action_t){2, 6, 0, 65535, 65535, 1, 1}, (game_action_t){16, 14, 35, 23, 65535, 1, 1}, (game_action_t){16, 15, 40, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 16, 39, 26, 65535, 1, 1}, (game_action_t){16, 18, 37, 27, 65535, 1, 1}, (game_action_t){16, 19, 37, 28, 65535, 1, 1}, (game_action_t){16, 20, 22, 29, 65535, 1, 1}, (game_action_t){16, 21, 34, 30, 65535, 1, 1}, (game_action_t){16, 22, 33, 31, 65535, 1, 1}, (game_action_t){16, 23, 37, 32, 65535, 1, 1}, (game_action_t){16, 24, 24, 65535, 65535, 1, 1}, (game_action_t){16, 25, 39, 34, 65535, 1, 1}, (game_action_t){16, 26, 39, 35, 65535, 1, 1}, (game_action_t){16, 27, 40, 36, 65535, 1, 1}, (game_action_t){16, 28, 33, 37, 65535, 1, 1}, (game_action_t){16, 29, 32, 38, 65535, 1, 1}, (game_action_t){16, 30, 36, 39, 65535, 1, 1}, (game_action_t){16, 31, 39, 40, 65535, 1, 1}, (game_action_t){16, 32, 40, 41, 65535, 1, 1}, (game_action_t){16, 33, 23, 42, 65535, 1, 1}, (game_action_t){16, 35, 24, 43, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 36, 25, 45, 65535, 1, 1}, (game_action_t){16, 38, 32, 46, 65535, 1, 1}, (game_action_t){16, 39, 40, 47, 65535, 1, 1}, (game_action_t){16, 40, 24, 48, 65535, 1, 1}, (game_action_t){16, 41, 28, 49, 65535, 1, 1}, (game_action_t){16, 42, 26, 65535, 65535, 1, 1}, (game_action_t){16, 43, 35, 51, 65535, 1, 1}, (game_action_t){16, 44, 40, 52, 65535, 1, 1}, (game_action_t){16, 45, 25, 65535, 65535, 1, 1}, (game_action_t){16, 47, 19, 57, 54, 1, 4}, (game_action_t){16, 48, 32, 57, 55, 1, 4}, (game_action_t){16, 49, 30, 57, 56, 1, 4}, (game_action_t){16, 50, 25, 57, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 47, 19, 62, 59, 1, 4}, (game_action_t){16, 48, 32, 62, 60, 1, 4}, (game_action_t){16, 49, 30, 62, 61, 1, 4}, (game_action_t){16, 50, 25, 62, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 65, 65535, 1, 1}, (game_action_t){16, 10, 36, 66, 65535, 1, 1}, (game_action_t){16, 11, 26, 67, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 4, 0, 65535, 65535, 1, 1}, (game_action_t){2, 5, 0, 65535, 65535, 1, 1}, (game_action_t){16, 53, 27, 73, 71, 1, 3}, (game_action_t){16, 54, 28, 73, 72, 1, 3}, (game_action_t){16, 55, 26, 73, 65535, 1, 3}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 53, 27, 77, 75, 1, 3}, (game_action_t){16, 54, 28, 77, 76, 1, 3}, (game_action_t){16, 55, 26, 77, 65535, 1, 3}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 56, 31, 80, 65535, 1, 1}, (game_action_t){16, 58, 25, 81, 65535, 1, 1}, (game_action_t){16, 4, 17, 82, 65535, 1, 1}, (game_action_t){16, 4, 17, 83, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 59, 27, 88, 85, 1, 4}, (game_action_t){16, 60, 25, 88, 86, 1, 4}, (game_action_t){16, 61, 24, 88, 87, 1, 4}, (game_action_t){16, 48, 32, 88, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 59, 27, 93, 90, 1, 4}, (game_action_t){16, 60, 25, 93, 91, 1, 4}, (game_action_t){16, 61, 24, 93, 92, 1, 4}, (game_action_t){16, 48, 32, 93, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 56, 31, 96, 65535, 1, 1}, (game_action_t){16, 58, 25, 97, 65535, 1, 1}, (game_action_t){16, 4, 17, 98, 65535, 1, 1}, (game_action_t){16, 4, 17, 99, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 4, 17, 101, 65535, 1, 1}, (game_action_t){16, 4, 17, 102, 65535, 1, 1}, (game_action_t){16, 4, 17, 103, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 63, 27, 108, 105, 1, 4}, (game_action_t){16, 64, 28, 108, 106, 1, 4}, (game_action_t){16, 50, 25, 108, 107, 1, 4}, (game_action_t){16, 65, 28, 108, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 63, 27, 113, 110, 1, 4}, (game_action_t){16, 64, 28, 113, 111, 1, 4}, (game_action_t){16, 50, 25, 113, 112, 1, 4}, (game_action_t){16, 65, 28, 113, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 56, 31, 116, 65535, 1, 1}, (game_action_t){16, 58, 25, 117, 65535, 1, 1}, (game_action_t){16, 4, 17, 118, 65535, 1, 1}, (game_action_t){16, 4, 17, 119, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 56, 31, 121, 65535, 1, 1}, (game_action_t){16, 58, 25, 122, 65535, 1, 1}, (game_action_t){16, 4, 17, 123, 65535, 1, 1}, (game_action_t){16, 4, 17, 124, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 68, 38, 126, 65535, 1, 1}, (game_action_t){16, 58, 25, 127, 65535, 1, 1}, (game_action_t){16, 4, 17, 128, 65535, 1, 1}, (game_action_t){16, 4, 17, 129, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 70, 26, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 71, 37, 133, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 71, 37, 135, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 71, 37, 137, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 71, 37, 139, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 7, 0, 65535, 65535, 1, 1}, (game_action_t){2, 8, 0, 65535, 65535, 1, 1}, (game_action_t){16, 78, 22, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 79, 32, 145, 65535, 1, 1}, (game_action_t){16, 80, 24, 146, 65535, 1, 1}, (game_action_t){16, 58, 25, 147, 65535, 1, 1}, (game_action_t){16, 4, 17, 148, 65535, 1, 1}, (game_action_t){16, 4, 17, 149, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 82, 39, 151, 65535, 1, 1}, (game_action_t){16, 58, 25, 152, 65535, 1, 1}, (game_action_t){16, 4, 17, 153, 65535, 1, 1}, (game_action_t){16, 4, 17, 154, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 84, 32, 156, 65535, 1, 1}, (game_action_t){16, 80, 24, 157, 65535, 1, 1}, (game_action_t){16, 58, 25, 158, 65535, 1, 1}, (game_action_t){16, 4, 17, 159, 65535, 1, 1}, (game_action_t){16, 4, 17, 160, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 86, 29, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 163, 65535, 1, 1}, (game_action_t){16, 10, 36, 164, 65535, 1, 1}, (game_action_t){16, 11, 26, 165, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 78, 22, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 82, 39, 169, 65535, 1, 1}, (game_action_t){16, 58, 25, 170, 65535, 1, 1}, (game_action_t){16, 4, 17, 171, 65535, 1, 1}, (game_action_t){16, 4, 17, 172, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 89, 25, 175, 65535, 1, 1}, (game_action_t){16, 58, 25, 176, 65535, 1, 1}, (game_action_t){16, 4, 17, 177, 65535, 1, 1}, (game_action_t){16, 4, 17, 178, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 91, 37, 180, 65535, 1, 1}, (game_action_t){16, 93, 27, 181, 65535, 1, 1}, (game_action_t){16, 4, 17, 182, 65535, 1, 1}, (game_action_t){16, 4, 17, 183, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 86, 29, 185, 65535, 1, 1}, (game_action_t){2, 9, 0, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 187, 65535, 1, 1}, (game_action_t){16, 10, 36, 188, 65535, 1, 1}, (game_action_t){16, 11, 26, 189, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 82, 39, 192, 65535, 1, 1}, (game_action_t){16, 58, 25, 193, 65535, 1, 1}, (game_action_t){16, 4, 17, 194, 65535, 1, 1}, (game_action_t){16, 4, 17, 195, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 89, 25, 198, 65535, 1, 1}, (game_action_t){16, 58, 25, 199, 65535, 1, 1}, (game_action_t){16, 4, 17, 200, 65535, 1, 1}, (game_action_t){16, 4, 17, 201, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 91, 37, 203, 65535, 1, 1}, (game_action_t){16, 93, 27, 204, 65535, 1, 1}, (game_action_t){16, 4, 17, 205, 65535, 1, 1}, (game_action_t){16, 4, 17, 206, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 86, 29, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 209, 65535, 1, 1}, (game_action_t){16, 10, 36, 210, 65535, 1, 1}, (game_action_t){16, 11, 26, 211, 65535, 1, 1}, (game_action_t){2, 10, 0, 65535, 65535, 1, 1}, (game_action_t){16, 95, 29, 213, 65535, 1, 1}, (game_action_t){16, 96, 20, 214, 65535, 1, 1}, (game_action_t){16, 97, 28, 215, 65535, 1, 1}, (game_action_t){16, 98, 32, 216, 65535, 1, 1}, (game_action_t){16, 99, 34, 217, 65535, 1, 1}, (game_action_t){16, 100, 21, 218, 65535, 1, 1}, (game_action_t){16, 4, 17, 219, 65535, 1, 1}, (game_action_t){16, 100, 21, 220, 65535, 1, 1}, (game_action_t){16, 4, 17, 221, 65535, 1, 1}, (game_action_t){16, 100, 21, 222, 65535, 1, 1}, (game_action_t){16, 4, 17, 223, 65535, 1, 1}, (game_action_t){16, 101, 26, 224, 65535, 1, 1}, (game_action_t){16, 102, 38, 225, 65535, 1, 1}, (game_action_t){16, 103, 21, 226, 65535, 1, 1}, (game_action_t){16, 104, 33, 227, 65535, 1, 1}, (game_action_t){16, 105, 36, 228, 65535, 1, 1}, (game_action_t){16, 106, 20, 229, 65535, 1, 1}, (game_action_t){2, 11, 0, 65535, 65535, 1, 1}, (game_action_t){16, 107, 28, 65535, 231, 1, 3}, (game_action_t){16, 108, 24, 65535, 232, 1, 3}, (game_action_t){16, 109, 32, 233, 65535, 1, 3}, (game_action_t){16, 110, 25, 65535, 65535, 1, 1}, (game_action_t){16, 111, 25, 237, 235, 1, 3}, (game_action_t){16, 112, 26, 237, 236, 1, 3}, (game_action_t){16, 113, 26, 237, 65535, 1, 3}, (game_action_t){16, 114, 27, 241, 238, 1, 4}, (game_action_t){16, 115, 32, 241, 239, 1, 4}, (game_action_t){16, 116, 34, 241, 240, 1, 4}, (game_action_t){16, 117, 31, 241, 65535, 1, 4}, (game_action_t){2, 31, 0, 65535, 65535, 1, 1}, (game_action_t){100, 0, 0, 243, 65535, 1, 1}, (game_action_t){18, 119, 26, 244, 65535, 1, 1}, (game_action_t){16, 120, 39, 245, 65535, 1, 1}, (game_action_t){18, 121, 40, 246, 65535, 1, 1}, (game_action_t){16, 122, 37, 247, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 123, 26, 251, 249, 1, 3}, (game_action_t){16, 125, 25, 251, 250, 1, 3}, (game_action_t){16, 126, 21, 251, 65535, 1, 3}, (game_action_t){16, 127, 34, 254, 252, 1, 3}, (game_action_t){16, 128, 38, 254, 253, 1, 3}, (game_action_t){16, 129, 36, 254, 65535, 1, 3}, (game_action_t){2, 11, 0, 65535, 65535, 1, 1}, (game_action_t){16, 130, 38, 256, 65535, 1, 1}, (game_action_t){16, 132, 39, 257, 65535, 1, 1}, (game_action_t){16, 133, 38, 258, 65535, 1, 1}, (game_action_t){16, 134, 36, 259, 65535, 1, 1}, (game_action_t){16, 135, 33, 260, 65535, 1, 1}, (game_action_t){2, 11, 0, 65535, 65535, 1, 1}, (game_action_t){16, 136, 27, 264, 262, 1, 3}, (game_action_t){16, 138, 34, 264, 263, 1, 3}, (game_action_t){16, 139, 35, 264, 65535, 1, 3}, (game_action_t){16, 140, 24, 267, 265, 1, 3}, (game_action_t){16, 141, 34, 267, 266, 1, 3}, (game_action_t){16, 142, 29, 267, 65535, 1, 3}, (game_action_t){16, 143, 30, 268, 65535, 1, 1}, (game_action_t){2, 11, 0, 65535, 65535, 1, 1}, (game_action_t){16, 111, 25, 273, 270, 1, 4}, (game_action_t){16, 112, 26, 273, 271, 1, 4}, (game_action_t){16, 144, 30, 273, 272, 1, 4}, (game_action_t){16, 145, 23, 273, 65535, 1, 4}, (game_action_t){16, 114, 27, 277, 274, 1, 4}, (game_action_t){16, 146, 30, 277, 275, 1, 4}, (game_action_t){16, 116, 34, 277, 276, 1, 4}, (game_action_t){16, 117, 31, 277, 65535, 1, 4}, (game_action_t){2, 31, 0, 65535, 65535, 1, 1}, (game_action_t){16, 147, 23, 65535, 279, 1, 4}, (game_action_t){16, 148, 36, 65535, 280, 1, 4}, (game_action_t){16, 149, 30, 65535, 281, 1, 4}, (game_action_t){18, 150, 37, 65535, 65535, 1, 4}, (game_action_t){16, 151, 37, 283, 65535, 1, 1}, (game_action_t){16, 152, 19, 284, 65535, 1, 1}, (game_action_t){16, 154, 37, 285, 65535, 1, 1}, (game_action_t){16, 155, 33, 286, 65535, 1, 1}, (game_action_t){16, 156, 36, 287, 65535, 1, 1}, (game_action_t){16, 157, 96, 288, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 158, 32, 290, 65535, 1, 1}, (game_action_t){16, 160, 33, 291, 292, 1, 3}, (game_action_t){16, 161, 24, 295, 65535, 1, 1}, (game_action_t){16, 162, 39, 295, 293, 1, 3}, (game_action_t){16, 163, 39, 294, 65535, 1, 3}, (game_action_t){16, 164, 20, 295, 65535, 1, 1}, (game_action_t){16, 165, 38, 296, 65535, 1, 1}, (game_action_t){17, 166, 25, 297, 65535, 1, 1}, (game_action_t){16, 167, 39, 298, 65535, 1, 1}, (game_action_t){16, 168, 24, 299, 65535, 1, 1}, (game_action_t){16, 169, 96, 300, 65535, 1, 1}, (game_action_t){16, 170, 96, 301, 65535, 1, 1}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 171, 35, 304, 303, 1, 2}, (game_action_t){16, 173, 28, 304, 65535, 1, 2}, (game_action_t){16, 174, 30, 307, 305, 1, 3}, (game_action_t){16, 175, 33, 307, 306, 1, 3}, (game_action_t){16, 176, 38, 307, 65535, 1, 3}, (game_action_t){16, 177, 32, 308, 65535, 1, 1}, (game_action_t){16, 178, 30, 310, 309, 1, 2}, (game_action_t){16, 179, 25, 310, 65535, 1, 2}, (game_action_t){16, 180, 96, 311, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 181, 29, 65535, 313, 1, 4}, (game_action_t){16, 182, 34, 65535, 314, 1, 4}, (game_action_t){16, 183, 37, 65535, 315, 1, 4}, (game_action_t){18, 150, 37, 65535, 65535, 1, 4}, (game_action_t){16, 184, 31, 317, 65535, 1, 1}, (game_action_t){16, 4, 17, 318, 65535, 1, 1}, (game_action_t){2, 32, 0, 65535, 65535, 1, 1}, (game_action_t){16, 185, 26, 322, 320, 1, 3}, (game_action_t){16, 187, 23, 322, 321, 1, 3}, (game_action_t){16, 188, 35, 322, 65535, 1, 3}, (game_action_t){16, 189, 32, 327, 323, 1, 4}, (game_action_t){16, 190, 31, 327, 324, 1, 4}, (game_action_t){16, 191, 31, 325, 326, 1, 4}, (game_action_t){16, 192, 26, 327, 65535, 1, 1}, (game_action_t){16, 193, 39, 327, 65535, 1, 4}, (game_action_t){16, 194, 33, 328, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 195, 21, 330, 65535, 1, 1}, (game_action_t){16, 197, 33, 331, 65535, 1, 1}, (game_action_t){16, 198, 40, 332, 65535, 1, 1}, (game_action_t){16, 199, 20, 333, 65535, 1, 1}, (game_action_t){16, 200, 36, 334, 65535, 1, 1}, (game_action_t){16, 201, 37, 335, 65535, 1, 1}, (game_action_t){16, 202, 39, 336, 65535, 1, 1}, (game_action_t){16, 4, 17, 337, 65535, 1, 1}, (game_action_t){16, 203, 33, 338, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 204, 35, 340, 65535, 1, 1}, (game_action_t){16, 206, 36, 341, 65535, 1, 1}, (game_action_t){16, 207, 37, 342, 65535, 1, 1}, (game_action_t){16, 4, 17, 343, 65535, 1, 1}, (game_action_t){16, 208, 24, 344, 65535, 1, 1}, (game_action_t){2, 14, 0, 65535, 65535, 1, 1}, (game_action_t){16, 147, 23, 346, 65535, 1, 1}, (game_action_t){16, 210, 34, 347, 65535, 1, 1}, (game_action_t){16, 211, 22, 348, 65535, 1, 1}, (game_action_t){2, 15, 0, 65535, 65535, 1, 1}, (game_action_t){1, 0, 0, 350, 65535, 1, 1}, (game_action_t){16, 212, 22, 351, 65535, 1, 1}, (game_action_t){16, 213, 27, 352, 65535, 1, 1}, (game_action_t){16, 214, 27, 353, 65535, 1, 1}, (game_action_t){16, 215, 27, 354, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){18, 216, 39, 356, 65535, 1, 1}, (game_action_t){16, 217, 25, 357, 65535, 1, 1}, (game_action_t){16, 219, 37, 358, 65535, 1, 1}, (game_action_t){16, 220, 25, 359, 65535, 1, 1}, (game_action_t){16, 221, 21, 360, 65535, 1, 1}, (game_action_t){1, 1, 0, 361, 65535, 1, 1}, (game_action_t){16, 222, 23, 362, 65535, 1, 1}, (game_action_t){16, 4, 17, 363, 65535, 1, 1}, (game_action_t){16, 223, 37, 364, 65535, 1, 1}, (game_action_t){6, 0, 0, 365, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 224, 25, 367, 65535, 1, 1}, (game_action_t){16, 225, 34, 368, 65535, 1, 1}, (game_action_t){16, 226, 26, 369, 65535, 1, 1}, (game_action_t){16, 227, 35, 370, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 228, 34, 372, 65535, 1, 1}, (game_action_t){16, 229, 24, 373, 65535, 1, 1}, (game_action_t){16, 231, 38, 374, 65535, 1, 1}, (game_action_t){16, 232, 24, 375, 65535, 1, 1}, (game_action_t){16, 233, 38, 376, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){16, 234, 29, 378, 65535, 1, 1}, (game_action_t){16, 236, 32, 379, 65535, 1, 1}, (game_action_t){16, 237, 36, 380, 65535, 1, 1}, (game_action_t){2, 17, 0, 65535, 65535, 1, 1}, (game_action_t){16, 234, 29, 382, 65535, 1, 1}, (game_action_t){16, 236, 32, 383, 65535, 1, 1}, (game_action_t){16, 237, 36, 384, 65535, 1, 1}, (game_action_t){2, 17, 0, 65535, 65535, 1, 1}, (game_action_t){16, 238, 34, 386, 65535, 1, 1}, (game_action_t){16, 239, 33, 387, 65535, 1, 1}, (game_action_t){16, 240, 38, 388, 65535, 1, 1}, (game_action_t){16, 241, 19, 389, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 242, 38, 391, 65535, 1, 1}, (game_action_t){16, 244, 35, 392, 65535, 1, 1}, (game_action_t){16, 245, 37, 393, 65535, 1, 1}, (game_action_t){16, 246, 35, 65535, 65535, 1, 1}, (game_action_t){16, 247, 20, 395, 65535, 1, 1}, (game_action_t){16, 249, 34, 396, 65535, 1, 1}, (game_action_t){16, 250, 40, 397, 65535, 1, 1}, (game_action_t){16, 251, 26, 398, 65535, 1, 1}, (game_action_t){16, 252, 36, 65535, 65535, 1, 1}, (game_action_t){16, 253, 26, 400, 65535, 1, 1}, (game_action_t){16, 255, 30, 401, 65535, 1, 1}, (game_action_t){16, 256, 40, 402, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){16, 257, 23, 407, 404, 1, 4}, (game_action_t){16, 258, 31, 407, 405, 1, 4}, (game_action_t){16, 259, 32, 407, 406, 1, 4}, (game_action_t){16, 260, 40, 407, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 409, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){16, 204, 35, 411, 65535, 1, 1}, (game_action_t){16, 206, 36, 412, 65535, 1, 1}, (game_action_t){16, 207, 37, 413, 65535, 1, 1}, (game_action_t){16, 4, 17, 414, 65535, 1, 1}, (game_action_t){16, 208, 24, 415, 65535, 1, 1}, (game_action_t){2, 14, 0, 65535, 65535, 1, 1}, (game_action_t){16, 261, 29, 417, 65535, 1, 1}, (game_action_t){16, 263, 37, 418, 65535, 1, 1}, (game_action_t){16, 264, 26, 419, 65535, 1, 1}, (game_action_t){16, 265, 28, 420, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 266, 32, 422, 65535, 1, 1}, (game_action_t){16, 268, 25, 428, 423, 1, 5}, (game_action_t){16, 269, 32, 428, 424, 1, 5}, (game_action_t){16, 270, 39, 425, 426, 1, 5}, (game_action_t){16, 271, 31, 428, 65535, 1, 1}, (game_action_t){16, 272, 31, 428, 427, 1, 5}, (game_action_t){16, 273, 31, 428, 65535, 1, 5}, (game_action_t){16, 274, 21, 431, 429, 1, 3}, (game_action_t){16, 275, 24, 431, 430, 1, 3}, (game_action_t){16, 276, 28, 431, 65535, 1, 3}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 277, 32, 433, 65535, 1, 1}, (game_action_t){16, 279, 34, 434, 65535, 1, 1}, (game_action_t){16, 280, 22, 435, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){1, 2, 0, 437, 65535, 1, 1}, (game_action_t){18, 281, 30, 65535, 65535, 1, 1}, (game_action_t){16, 282, 30, 439, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 283, 20, 65535, 441, 1, 4}, (game_action_t){18, 281, 30, 65535, 442, 1, 4}, (game_action_t){16, 284, 26, 65535, 443, 1, 4}, (game_action_t){16, 285, 27, 65535, 65535, 1, 4}, (game_action_t){16, 286, 32, 445, 65535, 1, 1}, (game_action_t){16, 288, 29, 446, 65535, 1, 1}, (game_action_t){16, 289, 40, 447, 65535, 1, 1}, (game_action_t){16, 290, 33, 448, 65535, 1, 1}, (game_action_t){16, 291, 35, 449, 65535, 1, 1}, (game_action_t){16, 292, 38, 450, 65535, 1, 1}, (game_action_t){6, 0, 0, 451, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 293, 40, 453, 65535, 1, 1}, (game_action_t){16, 294, 29, 454, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){16, 295, 35, 456, 65535, 1, 1}, (game_action_t){16, 297, 34, 458, 457, 1, 2}, (game_action_t){16, 298, 34, 458, 65535, 1, 2}, (game_action_t){16, 4, 17, 459, 65535, 1, 1}, (game_action_t){16, 299, 38, 460, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){16, 300, 27, 462, 65535, 1, 1}, (game_action_t){16, 4, 17, 463, 65535, 1, 1}, (game_action_t){16, 302, 30, 464, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){16, 303, 33, 466, 65535, 1, 1}, (game_action_t){16, 4, 17, 467, 65535, 1, 1}, (game_action_t){2, 21, 0, 65535, 65535, 1, 1}, (game_action_t){16, 305, 27, 469, 65535, 1, 1}, (game_action_t){16, 306, 24, 470, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){18, 307, 26, 472, 65535, 1, 1}, (game_action_t){16, 308, 36, 473, 65535, 1, 1}, (game_action_t){2, 22, 0, 65535, 65535, 1, 1}, (game_action_t){16, 309, 35, 475, 65535, 1, 1}, (game_action_t){16, 311, 40, 476, 65535, 1, 1}, (game_action_t){16, 312, 39, 477, 65535, 1, 1}, (game_action_t){2, 35, 0, 65535, 65535, 1, 1}, (game_action_t){16, 313, 160, 479, 65535, 1, 1}, (game_action_t){2, 22, 0, 65535, 65535, 1, 1}, (game_action_t){16, 314, 30, 481, 65535, 1, 1}, (game_action_t){16, 316, 36, 483, 482, 1, 2}, (game_action_t){16, 317, 33, 483, 65535, 1, 2}, (game_action_t){16, 318, 29, 485, 484, 1, 2}, (game_action_t){16, 319, 31, 485, 65535, 1, 2}, (game_action_t){16, 320, 30, 486, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 322, 31, 489, 65535, 1, 1}, (game_action_t){16, 324, 38, 490, 65535, 1, 1}, (game_action_t){16, 325, 40, 491, 65535, 1, 1}, (game_action_t){16, 326, 30, 492, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){16, 327, 39, 494, 65535, 1, 1}, (game_action_t){16, 329, 37, 495, 65535, 1, 1}, (game_action_t){16, 330, 40, 496, 65535, 1, 1}, (game_action_t){16, 331, 37, 497, 65535, 1, 1}, (game_action_t){16, 332, 20, 498, 65535, 1, 1}, (game_action_t){2, 23, 0, 65535, 65535, 1, 1}, (game_action_t){16, 333, 40, 500, 65535, 1, 1}, (game_action_t){16, 335, 37, 501, 65535, 1, 1}, (game_action_t){16, 336, 22, 502, 65535, 1, 1}, (game_action_t){16, 337, 39, 503, 65535, 1, 1}, (game_action_t){16, 338, 38, 504, 65535, 1, 1}, (game_action_t){16, 339, 38, 505, 65535, 1, 1}, (game_action_t){16, 340, 19, 506, 65535, 1, 1}, (game_action_t){2, 23, 0, 65535, 65535, 1, 1}, (game_action_t){16, 341, 35, 508, 65535, 1, 1}, (game_action_t){16, 343, 40, 509, 65535, 1, 1}, (game_action_t){16, 344, 33, 510, 65535, 1, 1}, (game_action_t){16, 345, 38, 511, 65535, 1, 1}, (game_action_t){16, 346, 21, 512, 65535, 1, 1}, (game_action_t){16, 347, 37, 513, 65535, 1, 1}, (game_action_t){16, 348, 21, 514, 65535, 1, 1}, (game_action_t){2, 23, 0, 65535, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 516, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 349, 22, 522, 519, 1, 4}, (game_action_t){16, 351, 28, 522, 520, 1, 4}, (game_action_t){16, 352, 32, 522, 521, 1, 4}, (game_action_t){16, 353, 23, 522, 65535, 1, 4}, (game_action_t){16, 354, 27, 524, 523, 1, 2}, (game_action_t){16, 355, 27, 524, 65535, 1, 2}, (game_action_t){19, 356, 34, 65535, 65535, 1, 1}, (game_action_t){16, 357, 24, 527, 526, 1, 2}, (game_action_t){16, 359, 19, 527, 65535, 1, 2}, (game_action_t){16, 360, 38, 528, 65535, 1, 1}, (game_action_t){2, 30, 0, 65535, 65535, 1, 1}, (game_action_t){2, 36, 0, 65535, 532, 4, 8}, (game_action_t){16, 361, 160, 531, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){16, 363, 21, 65535, 533, 1, 8}, (game_action_t){16, 364, 40, 534, 535, 1, 8}, (game_action_t){16, 365, 20, 65535, 65535, 1, 1}, (game_action_t){16, 366, 29, 65535, 536, 1, 8}, (game_action_t){16, 367, 37, 537, 65535, 1, 8}, (game_action_t){16, 368, 35, 538, 65535, 1, 1}, (game_action_t){16, 369, 24, 65535, 65535, 1, 1}, (game_action_t){2, 29, 0, 65535, 65535, 1, 1}, (game_action_t){2, 33, 0, 65535, 65535, 1, 1}, (game_action_t){16, 372, 27, 543, 542, 1, 2}, (game_action_t){16, 374, 35, 543, 65535, 1, 2}, (game_action_t){16, 375, 36, 544, 65535, 1, 1}, (game_action_t){16, 376, 32, 545, 65535, 1, 1}, (game_action_t){16, 226, 26, 546, 65535, 1, 1}, (game_action_t){2, 37, 0, 65535, 65535, 1, 1}, (game_action_t){16, 377, 160, 548, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){2, 34, 0, 65535, 65535, 1, 1}, (game_action_t){16, 349, 22, 555, 552, 1, 4}, (game_action_t){16, 351, 28, 555, 553, 1, 4}, (game_action_t){16, 352, 32, 555, 554, 1, 4}, (game_action_t){16, 353, 23, 555, 65535, 1, 4}, (game_action_t){16, 354, 27, 557, 556, 1, 2}, (game_action_t){16, 355, 27, 557, 65535, 1, 2}, (game_action_t){19, 356, 34, 65535, 65535, 1, 1}, (game_action_t){16, 357, 24, 560, 559, 1, 2}, (game_action_t){16, 359, 19, 560, 65535, 1, 2}, (game_action_t){16, 360, 38, 561, 65535, 1, 1}, (game_action_t){2, 30, 0, 65535, 65535, 1, 1}, (game_action_t){2, 36, 0, 65535, 563, 4, 7}, (game_action_t){16, 379, 32, 564, 565, 1, 7}, (game_action_t){16, 380, 25, 65535, 65535, 1, 1}, (game_action_t){16, 381, 40, 65535, 566, 1, 7}, (game_action_t){16, 382, 35, 65535, 65535, 1, 7}, (game_action_t){16, 372, 27, 569, 568, 1, 2}, (game_action_t){16, 374, 35, 569, 65535, 1, 2}, (game_action_t){16, 375, 36, 570, 65535, 1, 1}, (game_action_t){16, 376, 32, 571, 65535, 1, 1}, (game_action_t){16, 226, 26, 572, 65535, 1, 1}, (game_action_t){2, 37, 0, 65535, 65535, 1, 1}, (game_action_t){16, 383, 23, 574, 65535, 1, 1}, (game_action_t){16, 144, 30, 575, 65535, 1, 1}, (game_action_t){16, 385, 31, 576, 65535, 1, 1}, (game_action_t){2, 28, 0, 65535, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){2, 38, 0, 65535, 65535, 1, 1}, (game_action_t){16, 386, 160, 580, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){2, 39, 0, 65535, 65535, 1, 1}, (game_action_t){16, 388, 160, 583, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 349, 22, 588, 585, 1, 4}, (game_action_t){16, 351, 28, 588, 586, 1, 4}, (game_action_t){16, 352, 32, 588, 587, 1, 4}, (game_action_t){16, 353, 23, 588, 65535, 1, 4}, (game_action_t){16, 354, 27, 590, 589, 1, 2}, (game_action_t){16, 355, 27, 590, 65535, 1, 2}, (game_action_t){19, 356, 34, 65535, 65535, 1, 1}, (game_action_t){16, 357, 24, 593, 592, 1, 2}, (game_action_t){16, 359, 19, 593, 65535, 1, 2}, (game_action_t){16, 360, 38, 594, 65535, 1, 1}, (game_action_t){2, 30, 0, 65535, 65535, 1, 1}, (game_action_t){2, 36, 0, 65535, 596, 4, 5}, (game_action_t){16, 390, 27, 65535, 65535, 1, 5}, (game_action_t){16, 372, 27, 599, 598, 1, 2}, (game_action_t){16, 374, 35, 599, 65535, 1, 2}, (game_action_t){16, 375, 36, 600, 65535, 1, 1}, (game_action_t){16, 376, 32, 601, 65535, 1, 1}, (game_action_t){16, 226, 26, 602, 65535, 1, 1}, (game_action_t){2, 37, 0, 65535, 65535, 1, 1}, (game_action_t){100, 1, 0, 604, 65535, 1, 1}, (game_action_t){16, 391, 31, 65535, 65535, 1, 1}, (game_action_t){16, 392, 28, 606, 65535, 1, 1}, (game_action_t){16, 394, 38, 607, 65535, 1, 1}, (game_action_t){16, 395, 21, 608, 65535, 1, 1}, (game_action_t){16, 396, 38, 609, 65535, 1, 1}, (game_action_t){16, 397, 37, 610, 65535, 1, 1}, (game_action_t){16, 398, 27, 611, 65535, 1, 1}, (game_action_t){16, 399, 39, 612, 65535, 1, 1}, (game_action_t){16, 400, 31, 613, 65535, 1, 1}, (game_action_t){6, 0, 0, 614, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 401, 33, 616, 65535, 1, 1}, (game_action_t){16, 402, 32, 617, 65535, 1, 1}, (game_action_t){16, 403, 38, 618, 65535, 1, 1}, (game_action_t){16, 404, 30, 619, 65535, 1, 1}, (game_action_t){6, 0, 0, 620, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 405, 30, 622, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 406, 26, 625, 624, 1, 2}, (game_action_t){16, 407, 29, 625, 65535, 1, 2}, (game_action_t){19, 408, 35, 65535, 626, 1, 3}, (game_action_t){19, 409, 28, 65535, 627, 1, 3}, (game_action_t){19, 410, 30, 65535, 65535, 1, 3}, (game_action_t){16, 411, 21, 631, 629, 1, 3}, (game_action_t){16, 413, 24, 631, 630, 1, 3}, (game_action_t){16, 414, 31, 631, 65535, 1, 3}, (game_action_t){16, 4, 96, 65535, 65535, 1, 1}, (game_action_t){16, 415, 25, 633, 65535, 1, 1}, (game_action_t){16, 416, 33, 634, 65535, 1, 1}, (game_action_t){16, 417, 30, 635, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 418, 21, 640, 637, 1, 3}, (game_action_t){16, 419, 40, 640, 638, 1, 3}, (game_action_t){16, 420, 40, 639, 65535, 1, 3}, (game_action_t){16, 421, 27, 640, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 422, 31, 642, 65535, 1, 1}, (game_action_t){16, 423, 34, 643, 65535, 1, 1}, (game_action_t){16, 424, 36, 644, 65535, 1, 1}, (game_action_t){16, 425, 23, 645, 65535, 1, 1}, (game_action_t){16, 426, 29, 646, 65535, 1, 1}, (game_action_t){16, 427, 37, 647, 65535, 1, 1}, (game_action_t){16, 4, 17, 648, 65535, 1, 1}, (game_action_t){16, 428, 38, 649, 65535, 1, 1}, (game_action_t){6, 0, 0, 650, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 429, 32, 652, 65535, 1, 1}, (game_action_t){16, 430, 35, 653, 65535, 1, 1}, (game_action_t){16, 431, 26, 654, 65535, 1, 1}, (game_action_t){16, 432, 40, 655, 65535, 1, 1}, (game_action_t){16, 433, 38, 656, 65535, 1, 1}, (game_action_t){16, 434, 28, 657, 65535, 1, 1}, (game_action_t){6, 0, 0, 658, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}};

game_state_t all_states[] = {(game_state_t){.entry_series_id=0, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=5}}, .input_series={(game_user_in_t){.text_addr=5, .result_action_id=10},(game_user_in_t){.text_addr=6, .result_action_id=11},(game_user_in_t){.text_addr=7, .result_action_id=12},(game_user_in_t){.text_addr=9, .result_action_id=16},(game_user_in_t){.text_addr=12, .result_action_id=20},(game_user_in_t){.text_addr=13, .result_action_id=21}}, .other_series={}}, (game_state_t){.entry_series_id=22, .timer_series_len=1, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=24}}, .input_series={(game_user_in_t){.text_addr=17, .result_action_id=25},(game_user_in_t){.text_addr=34, .result_action_id=33},(game_user_in_t){.text_addr=37, .result_action_id=44},(game_user_in_t){.text_addr=46, .result_action_id=50}}, .other_series={}}, (game_state_t){.entry_series_id=53, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=58},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=63}}, .input_series={(game_user_in_t){.text_addr=9, .result_action_id=64},(game_user_in_t){.text_addr=51, .result_action_id=68},(game_user_in_t){.text_addr=52, .result_action_id=69}}, .other_series={}}, (game_state_t){.entry_series_id=70, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=74},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=78}}, .input_series={(game_user_in_t){.text_addr=57, .result_action_id=79}}, .other_series={}}, (game_state_t){.entry_series_id=84, .timer_series_len=2, .input_series_len=2, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=89},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=94}}, .input_series={(game_user_in_t){.text_addr=62, .result_action_id=95},(game_user_in_t){.text_addr=7, .result_action_id=100}}, .other_series={}}, (game_state_t){.entry_series_id=104, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=109},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=114}}, .input_series={(game_user_in_t){.text_addr=66, .result_action_id=115},(game_user_in_t){.text_addr=67, .result_action_id=120},(game_user_in_t){.text_addr=69, .result_action_id=125}}, .other_series={}}, (game_state_t){.entry_series_id=130, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=131}}, .input_series={(game_user_in_t){.text_addr=72, .result_action_id=132},(game_user_in_t){.text_addr=73, .result_action_id=134},(game_user_in_t){.text_addr=74, .result_action_id=136},(game_user_in_t){.text_addr=75, .result_action_id=138},(game_user_in_t){.text_addr=76, .result_action_id=140},(game_user_in_t){.text_addr=77, .result_action_id=141}}, .other_series={}}, (game_state_t){.entry_series_id=142, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=143}}, .input_series={(game_user_in_t){.text_addr=81, .result_action_id=144},(game_user_in_t){.text_addr=83, .result_action_id=150},(game_user_in_t){.text_addr=85, .result_action_id=155},(game_user_in_t){.text_addr=87, .result_action_id=161},(game_user_in_t){.text_addr=9, .result_action_id=162}}, .other_series={}}, (game_state_t){.entry_series_id=166, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=167}}, .input_series={(game_user_in_t){.text_addr=83, .result_action_id=168},(game_user_in_t){.text_addr=88, .result_action_id=173},(game_user_in_t){.text_addr=90, .result_action_id=174},(game_user_in_t){.text_addr=92, .result_action_id=179},(game_user_in_t){.text_addr=94, .result_action_id=184},(game_user_in_t){.text_addr=9, .result_action_id=186}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=190}}, .input_series={(game_user_in_t){.text_addr=83, .result_action_id=191},(game_user_in_t){.text_addr=88, .result_action_id=196},(game_user_in_t){.text_addr=90, .result_action_id=197},(game_user_in_t){.text_addr=92, .result_action_id=202},(game_user_in_t){.text_addr=94, .result_action_id=207},(game_user_in_t){.text_addr=9, .result_action_id=208}}, .other_series={}}, (game_state_t){.entry_series_id=212, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=230},(game_timer_t){.duration=1152, .recurring=0, .result_action_id=234}}, .input_series={(game_user_in_t){.text_addr=118, .result_action_id=242},(game_user_in_t){.text_addr=124, .result_action_id=248},(game_user_in_t){.text_addr=131, .result_action_id=255},(game_user_in_t){.text_addr=137, .result_action_id=261}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=1152, .recurring=0, .result_action_id=269},(game_timer_t){.duration=256, .recurring=0, .result_action_id=278}}, .input_series={(game_user_in_t){.text_addr=153, .result_action_id=282},(game_user_in_t){.text_addr=159, .result_action_id=289},(game_user_in_t){.text_addr=172, .result_action_id=302}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=480, .recurring=0, .result_action_id=312},(game_timer_t){.duration=38400, .recurring=0, .result_action_id=316}}, .input_series={(game_user_in_t){.text_addr=186, .result_action_id=319},(game_user_in_t){.text_addr=196, .result_action_id=329},(game_user_in_t){.text_addr=205, .result_action_id=339},(game_user_in_t){.text_addr=209, .result_action_id=345}}, .other_series={}}, (game_state_t){.entry_series_id=349, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=218, .result_action_id=355}}, .other_series={}}, (game_state_t){.entry_series_id=366, .timer_series_len=0, .input_series_len=2, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=230, .result_action_id=371},(game_user_in_t){.text_addr=235, .result_action_id=377}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=235, .result_action_id=381}}, .other_series={}}, (game_state_t){.entry_series_id=385, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=243, .result_action_id=390},(game_user_in_t){.text_addr=248, .result_action_id=394},(game_user_in_t){.text_addr=254, .result_action_id=399}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=403},(game_timer_t){.duration=90240, .recurring=0, .result_action_id=408}}, .input_series={(game_user_in_t){.text_addr=205, .result_action_id=410},(game_user_in_t){.text_addr=262, .result_action_id=416},(game_user_in_t){.text_addr=267, .result_action_id=421},(game_user_in_t){.text_addr=278, .result_action_id=432}}, .other_series={}}, (game_state_t){.entry_series_id=436, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=960, .recurring=0, .result_action_id=438},(game_timer_t){.duration=64, .recurring=0, .result_action_id=440}}, .input_series={(game_user_in_t){.text_addr=287, .result_action_id=444}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=452}}, .input_series={(game_user_in_t){.text_addr=296, .result_action_id=455},(game_user_in_t){.text_addr=301, .result_action_id=461},(game_user_in_t){.text_addr=304, .result_action_id=465}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=0, .other_series_len=2, .timer_series={}, .input_series={}, .other_series={(game_other_in_t){.type_id=0, .result_action_id=468},(game_other_in_t){.type_id=1, .result_action_id=471}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=310, .result_action_id=474}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=315, .result_action_id=480},(game_user_in_t){.text_addr=321, .result_action_id=487},(game_user_in_t){.text_addr=323, .result_action_id=488}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=328, .result_action_id=493},(game_user_in_t){.text_addr=334, .result_action_id=499},(game_user_in_t){.text_addr=342, .result_action_id=507}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=9600, .recurring=0, .result_action_id=515},(game_timer_t){.duration=21120, .recurring=0, .result_action_id=517}}, .input_series={(game_user_in_t){.text_addr=350, .result_action_id=518},(game_user_in_t){.text_addr=358, .result_action_id=525},(game_user_in_t){.text_addr=362, .result_action_id=529},(game_user_in_t){.text_addr=370, .result_action_id=539},(game_user_in_t){.text_addr=371, .result_action_id=540},(game_user_in_t){.text_addr=373, .result_action_id=541}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=549}}, .input_series={(game_user_in_t){.text_addr=378, .result_action_id=550},(game_user_in_t){.text_addr=350, .result_action_id=551},(game_user_in_t){.text_addr=358, .result_action_id=558},(game_user_in_t){.text_addr=362, .result_action_id=562},(game_user_in_t){.text_addr=373, .result_action_id=567},(game_user_in_t){.text_addr=384, .result_action_id=573}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=577}}, .input_series={(game_user_in_t){.text_addr=387, .result_action_id=578},(game_user_in_t){.text_addr=389, .result_action_id=581},(game_user_in_t){.text_addr=350, .result_action_id=584},(game_user_in_t){.text_addr=358, .result_action_id=591},(game_user_in_t){.text_addr=362, .result_action_id=595},(game_user_in_t){.text_addr=373, .result_action_id=597}}, .other_series={}}, (game_state_t){.entry_series_id=603, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=393, .result_action_id=605}}, .other_series={}}, (game_state_t){.entry_series_id=615, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=623, .timer_series_len=1, .input_series_len=0, .other_series_len=0, .timer_series={(game_timer_t){.duration=320, .recurring=0, .result_action_id=621}}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=632}}, .input_series={(game_user_in_t){.text_addr=412, .result_action_id=628}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=412, .result_action_id=636}}, .other_series={}}, (game_state_t){.entry_series_id=641, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=651, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=478, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=530, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=547, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=579, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=582, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}};

#define SPECIAL_BADGESNEARBY0 0
#define SPECIAL_BADGESNEARBYSOME 1
#define OTHER_ACTION_CUSTOMSTATEUSERNAME 0
#define OTHER_ACTION_TURN_ON_THE_LIGHTS_TO_REPRESENT_FILE_STATE 1
#define CLOSABLE_STATES 6


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
        HT16D_BRIGHTNESS_DEFAULT,
        LED_ANIM_TYPE_SPIN,
};

const rgbcolor_t lsw_colors[] = {
        {255, 255, 255},
        {255, 255, 255},
        {255, 255, 255},
        {255, 255, 255},
        {255, 255, 255},
        {255, 255, 255},
};

const led_ring_animation_t anim_lsw = { // "light solid white" (bright)
        &lsw_colors[0],
        6,
        72,
        HT16D_BRIGHTNESS_MAX,
        LED_ANIM_TYPE_SAME,
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
    lcd111_clear(LCD_TOP);

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
        if (current_state->other_series[i].type_id == SPECIAL_BADGESNEARBYSOME &&
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
    all_animations[0] = anim_lsw;
    all_animations[1] = anim_lwf;
    all_animations[2] = anim_rainbow;

    game_set_state(20);
}

/// Render bottom screen for the current state and value of `text_selection`.
void draw_text_selection() {
    lcd111_cursor_pos(LCD_BTM, 0);
    lcd111_put_char(LCD_BTM, 0xBB); // This is &raquo;
    if (text_selection) {
        lcd111_cursor_type(LCD_BTM, LCD111_CURSOR_NONE);
        lcd111_put_text_pad(
                LCD_BTM,
                all_text[current_state->input_series[text_selection-1].text_addr],
                23
        );
    } else {
        // Haven't used an arrow key yet.
        if (current_state->input_series_len) {
            lcd111_put_text_pad(LCD_BTM, "", 21);
            lcd111_put_text_pad(LCD_BTM, "\x1E\x1F", 2);
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
        led_set_anim(&all_animations[action->detail], 0, 0, 0);
        break;
    case GAME_ACTION_TYPE_SET_ANIM_BG:
        // Set a new background animation
        if (action->detail >= all_anims_len || action->detail == GAME_NULL) {
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

    lcd111_cursor_type(LCD_BTM, LCD111_CURSOR_NONE);

    // Now, we know we're dealing with a real action series.
    // Place the appropriate action choice into `loaded_action`.
    select_action_choice(action_id);
    // We know we're in an action series, so set everything up:
    in_action_series = 1;
    game_curr_elapsed = 0;
    lcd111_set_text(0, ""); // TODO
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
        text_selection += 1;
        if (text_selection == current_state->input_series_len+1)
            text_selection = 1;
    } while (leads_to_closed_state(current_state->input_series[text_selection-1].result_action_id));
    draw_text_selection();
}

void prev_input() {
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
