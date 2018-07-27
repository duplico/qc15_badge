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


// The following data will be loaded from the script:
led_ring_animation_t all_animations[4];

uint8_t game_name_buffer[QC15_BADGE_NAME_LEN];

uint16_t all_actions_len = 798;
uint16_t all_text_len = 506;
uint16_t all_states_len = 49;
uint16_t all_anims_len = 4;

// lightsSolidWhite, lightsWhiteFader, animSpinBlue, whiteDiscovery
uint8_t all_text[][25] = {"wDFGHffdf","234vv33vdf","Cucumber","e99tftgED","","Hello?","Are you broken?","Reboot.","INITIALIZING","Initialize.","....................","Completed.","I'm confused","Set state.","Scan what, %s?","Nah, nothing.","But something was there.","I'm so confused.","WOAH. WOAH.","So I'm not alone.","(No offense)","Never believe what I","learned.","Welcome to customer","support, how may I help?","I'm sorry to hear that!","Badge doesn't work.","I'll be glad to help.","First we need to do a","reset.","Please unplug your","badge, wait three","seconds, then plug it","back in.","I'm sorry I have failed","to provide you adequate","assistance at this time.","Our supervisor is","available on 3rd","Tuesdays between the","hours of 3am and 3:17am","in whatever timezone you","are in.","Supervisor","Goodbye.","Have you?","I tried a reboot.","Have you really?","You think you're smarter","than me?","Do it again.","I'll wait.","I'm sorry, I cannot","discuss personal matters","with you.","You speak English!","...","Hello(gibberish)","HeGibberishllo","Gibberish","Speak English!","Learn","broke455gfr","you broken6!","Are you...","BUFFER OVERFLOW","I give up.","Rebooting","Speak hello","Engli-llo","Speakeng","Getting closer...","Words learn","Heword learn","Earn wordslo","Keep learning","Load English","ERR: Not in state mode","Store wordset","Set state:","ERR: State not found.","Language","Mode","First Run","AR112-1","ISO-3166-1.2","BCP-47","Value:","ERR: \"You pick.\"","invalid.","You pick.","ERR: \"English\" invalid.","English","ERR: \"Game mode\"","Game mode","Value stored.","US","Help","You wish.","God mode","Datei nicht gefunden.","zh-DE","Neustart...","en-boont-latn-021","Hello! I am -","....","Waitaminute.","What's going on?","Sysclock just says","12:00","Ohh, crap.","I've lost main memory.","CRAP!","This is not good.","And who the hell are","you?","Still there?","Hellooo?","Not a trick question.","Screw it.","That's it.","Nevermind.","We're done.","Waste my time...","You're killing me.","See you, human.","My name is...","%s?","Good name for a monkey.","Well %s I have no","idea what's going on.","Seriously?","Who are you?","For real?","What?","I asked you first.","You don't question ME.","You hard of hearing?","For crying out loud...","What are you?","I'm not a *thing*. Well","ok, I mean... shut up.","You going to tell me","your name or not?","Everything!","What do you do?","I am all-powerful!","I control a file...","I think.","Do you believe me?","Or something.","No matter now.","You know what?","Helloo?","Waste my time.","Sigh...","I need more buttons.","So frustrated.","You there, %s?","It's like I just woke up","What do you mean?","I was doing something.","Holding something.","Something important.","But now I don't know.","Ah, this I know.","What's your name?","Some call me \"Mr.","Awesome\"","Some call me \"Mysterio\"","Some call me \"The Great","One\"","But you can call me...","%s","Now we know each other.","Sort of.","Wish I knew more than","that.","WHAT. DID YOU. SAY?","You're a badge.","No I am NOT.","I'm brilliant.","I'm all-powerful.","I have one fine booty.","And I need help.","From a monkey.","From you.","This SUCKS.","I feel stuck.","Do you smell that?","Reboots are a killer.","Ok, later then.","My status?","Status","Who me?","You're curious, eh.","Damned confused.","Stuck with you.","Dark, lost, and","brilliant.","Dumbstruck by a reboot.","That's my status.","WHAT?","You're my badge!","You don't OWN me!","If anything, I would own","YOU.","All superior beings,","raise your processing","power to 6.77 exawatts.","Yeah, thought so.","Do YOU have lights?","Do you have lights?","See how that sounds?","That's how you sound.","Oh wait.","What do you need?","Guess I'm glad you","asked.","WOHOO!","I did that.","Me. Booyah!","So awesome.","Ugh %s that's so","like you.","That's really bright.","I do a thing, and you","complain.","Fine.","Better?","Ok yes that's better.","I'm lost.","I remember holding","something.","And I wasn't alone.","Is that so hard to","believe?","Like friends?","I'll have you know I'm","awesome.","But now I have no one.","Oh, monkey...","Hey, I'm here.","You don't count.","I mean more like me.","I've lost control.","And lost my goal.","Somehow, you must help","me.","I protected something.","What was your goal?","Part of it, anyway.","I assume I still have","it, whatever it is.","HEY!","What's in it for me?","There's gotta be a","\"shock the idiot\" button","somewhere.","Try pushing buttons.","Thank you!","Ok, I'll help.","One problem...","I don't know what to do.","Humm...","I am awesome...","Soooo awesome...","The awesome is meee.....","I'm not sure.","What's your goal?","I'm sensing something","out there.","And in here.","More? Like what?","Can you do more?","Brew tea?","Make a sandwich?","I dunno, try putting me","in the freezer.","Give back rubs?","Tie your shoes?","Dolt.","Dumbass.","Seriously...","Scan? Scan what?","Scan","You, me, or... out","there?","Hey %s!","Eh, nevermind.","Hey!","Seriously!","Guess what?","I just realized!","What is it?","There's no 2!","In my code, there are 1s","and 0s but no 2s.","Can you count to 2?","Don't strain yourself.","Ok, I scanned your face.","It was funny.","Uh, ok. Scanning...","You.","Lots of awesome...","A hint of basil...","...and something else.","Really? Ok.","Me.","Nope. Nothing.","Right. Looking...","Out there.","...nothing.","Oh well.","%s.","Someone's out there.","Connecting to what?","Try connecting","There are some markings.","Actually, yes.","Anything about me?","You're funny-lookin.","You have 11 toes.","But are nice?","But smell good?","I'm skeptical.","Tell me about...","I sure hope so.","Got it. Let's do it.","If not, ask other meat","bags with more patience.","Let's do this!","Another being, like me.","...that badge?","I can see names, now.","And we have something...","...a file. Or part of","one.","It seems there's a file.","...your goal?","Broken up. I have one","piece.","Others have pieces too.","But they're encrypted.","Together, we can crack","it.","Well I'm not alone.","...you?","I protect this file, but","it is fragmented.","And locked. No sign of","keys.","I have to brute force","this.","Why's the file locked?","Do you have speakers?","Dunno.","Anyone around?","Let's check.","Why, you lonely?","Hrmm...","Scanning...","Checking...","I see %d badges.","Giggity.","Let's connect.","Ok.","Connecting to network.","CUSTOMSTATESTATUS","Status check","Busy!","A little hungry. Is that","odd?","Uff... tired.","Wondering if it's too","late to turn into a","toaster.","More bling?","More with lights?","Not gay enough yet?","I'll try some things","Shout if you see","COLORFINDER","How old are you?","Studying quantum","knitting.","Practicing my showtunes.","What's YOUR status?","It's...","How's the file?","Let's try this.","Do you have it all?","No, I don't.","By my calculations, I","have 1/16th of the total","file.","I see other bits when we","connect to others.","At some point, we all","need to come together.","Hey $badgname","Don't be afraid.","I really like your","daughter.","Out there in the cold.","Gonna be %s.","You're a rock star.","Soul %s.","What IS the file?","No matter, the file is","something special to","some people.","Yes","No","%s the awesome.","Who's asking?","Your mother.","Me, you dolt!","Definitely not China.","Anyway, it's special to","QUEERCON.","How well do you know em?","JUSTJOINED","Just joined.","From the beginning.","It's been a while.","Well, they're like this","exclusive club.","Sorry, I mean","\"inclusive\".","Anyway, this file is","like their long lost","sigil.","I don't know anything","more.","Well, welcome.","Glad you're here.","Meat bags like you are","so handy.","from them.","It's like a mascot from","long ago.","Even I've never seen it.","So let's crack it!","Really???","Don't call me badge!","Watch it, badge!","Really need to install","that \"shock the idiot\"","button...","Anyway, the file is the","original sigil, I hear.","So you should know it.","Mr. \"Been here forever\".","The smartest!","Smartass.","Finally a compliment.","Impressive!","I think 3 days with you","is my limit.","In the beginning, the","founders set a mascot.","I think that's the file.","Does that help?","So no, then.","What am I seeing?","Each LED is part of my","The more we unlock the","file, the more stable","the lights.","Also other stuff, go to","the badge talk.","There's a note...","\"To make closing","ceremony interesting.\"","Mean anything?","Logging off...","Connected!","We're online.","%d badges around.","%d others.","%d in range.","Nope.","Wake up!","Go away.","I'm still hurt.","Ok, fine.","Now you be nicer!","Where were we?","Wha-?","Beep beep. Just kidding.","Ugh, I was having such a","nice dream.","...oh. My. GOD.","You want speakers?","What are we playing?","Bieber?","Fraggle Rock?","A capella hair metal?","I only play showtunes.","First off, rude.","Second, still rude.","I dunno...","About 15K of your years.","But younger than those","who made me."};

game_action_t all_actions[] = {(game_action_t){16, 0, 25, 65535, 1, 1, 4}, (game_action_t){16, 1, 26, 65535, 2, 1, 4}, (game_action_t){16, 2, 24, 65535, 3, 1, 4}, (game_action_t){16, 3, 25, 65535, 65535, 1, 4}, (game_action_t){16, 0, 25, 8, 5, 1, 4}, (game_action_t){16, 1, 26, 8, 6, 1, 4}, (game_action_t){16, 2, 24, 8, 7, 1, 4}, (game_action_t){16, 3, 25, 8, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){2, 3, 0, 65535, 65535, 1, 1}, (game_action_t){2, 4, 0, 65535, 65535, 1, 1}, (game_action_t){16, 4, 17, 12, 65535, 1, 1}, (game_action_t){16, 4, 17, 13, 65535, 1, 1}, (game_action_t){16, 4, 17, 14, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 16, 65535, 1, 1}, (game_action_t){16, 10, 36, 17, 65535, 1, 1}, (game_action_t){16, 11, 26, 18, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 65535, 1, 1}, (game_action_t){2, 7, 0, 65535, 65535, 1, 1}, (game_action_t){18, 14, 37, 22, 65535, 1, 1}, (game_action_t){100, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 15, 29, 24, 65535, 1, 1}, (game_action_t){16, 16, 40, 25, 65535, 1, 1}, (game_action_t){16, 17, 32, 26, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 18, 27, 28, 65535, 1, 1}, (game_action_t){0, 2, 160, 29, 65535, 1, 1}, (game_action_t){16, 19, 33, 30, 65535, 1, 1}, (game_action_t){16, 20, 28, 31, 65535, 1, 1}, (game_action_t){16, 21, 36, 32, 65535, 1, 1}, (game_action_t){16, 22, 24, 33, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 23, 35, 35, 65535, 1, 1}, (game_action_t){16, 24, 40, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 25, 39, 38, 65535, 1, 1}, (game_action_t){16, 27, 37, 39, 65535, 1, 1}, (game_action_t){16, 28, 37, 40, 65535, 1, 1}, (game_action_t){16, 29, 22, 41, 65535, 1, 1}, (game_action_t){16, 30, 34, 42, 65535, 1, 1}, (game_action_t){16, 31, 33, 43, 65535, 1, 1}, (game_action_t){16, 32, 37, 44, 65535, 1, 1}, (game_action_t){16, 33, 24, 65535, 65535, 1, 1}, (game_action_t){16, 34, 39, 46, 65535, 1, 1}, (game_action_t){16, 35, 39, 47, 65535, 1, 1}, (game_action_t){16, 36, 40, 48, 65535, 1, 1}, (game_action_t){16, 37, 33, 49, 65535, 1, 1}, (game_action_t){16, 38, 32, 50, 65535, 1, 1}, (game_action_t){16, 39, 36, 51, 65535, 1, 1}, (game_action_t){16, 40, 39, 52, 65535, 1, 1}, (game_action_t){16, 41, 40, 53, 65535, 1, 1}, (game_action_t){16, 42, 23, 54, 65535, 1, 1}, (game_action_t){16, 44, 24, 55, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 45, 25, 57, 65535, 1, 1}, (game_action_t){16, 47, 32, 58, 65535, 1, 1}, (game_action_t){16, 48, 40, 59, 65535, 1, 1}, (game_action_t){16, 49, 24, 60, 65535, 1, 1}, (game_action_t){16, 50, 28, 61, 65535, 1, 1}, (game_action_t){16, 51, 26, 65535, 65535, 1, 1}, (game_action_t){16, 52, 35, 63, 65535, 1, 1}, (game_action_t){16, 53, 40, 64, 65535, 1, 1}, (game_action_t){16, 54, 25, 65535, 65535, 1, 1}, (game_action_t){16, 56, 19, 65535, 66, 1, 4}, (game_action_t){16, 57, 32, 65535, 67, 1, 4}, (game_action_t){16, 58, 30, 65535, 68, 1, 4}, (game_action_t){16, 59, 25, 65535, 65535, 1, 4}, (game_action_t){16, 56, 19, 65535, 70, 1, 4}, (game_action_t){16, 57, 32, 65535, 71, 1, 4}, (game_action_t){16, 58, 30, 65535, 72, 1, 4}, (game_action_t){16, 59, 25, 65535, 65535, 1, 4}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 75, 65535, 1, 1}, (game_action_t){16, 10, 36, 76, 65535, 1, 1}, (game_action_t){16, 11, 26, 77, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 5, 0, 65535, 65535, 1, 1}, (game_action_t){2, 6, 0, 65535, 65535, 1, 1}, (game_action_t){16, 62, 27, 65535, 81, 1, 3}, (game_action_t){16, 63, 28, 65535, 82, 1, 3}, (game_action_t){16, 64, 26, 65535, 65535, 1, 3}, (game_action_t){16, 62, 27, 65535, 84, 1, 3}, (game_action_t){16, 63, 28, 65535, 85, 1, 3}, (game_action_t){16, 64, 26, 65535, 65535, 1, 3}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 65, 31, 88, 65535, 1, 1}, (game_action_t){16, 67, 25, 89, 65535, 1, 1}, (game_action_t){16, 4, 17, 90, 65535, 1, 1}, (game_action_t){16, 4, 17, 91, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 68, 27, 65535, 93, 1, 4}, (game_action_t){16, 69, 25, 65535, 94, 1, 4}, (game_action_t){16, 70, 24, 65535, 95, 1, 4}, (game_action_t){16, 57, 32, 65535, 65535, 1, 4}, (game_action_t){16, 68, 27, 65535, 97, 1, 4}, (game_action_t){16, 69, 25, 65535, 98, 1, 4}, (game_action_t){16, 70, 24, 65535, 99, 1, 4}, (game_action_t){16, 57, 32, 65535, 65535, 1, 4}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 65, 31, 102, 65535, 1, 1}, (game_action_t){16, 67, 25, 103, 65535, 1, 1}, (game_action_t){16, 4, 17, 104, 65535, 1, 1}, (game_action_t){16, 4, 17, 105, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 4, 17, 107, 65535, 1, 1}, (game_action_t){16, 4, 17, 108, 65535, 1, 1}, (game_action_t){16, 4, 17, 109, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 72, 27, 65535, 111, 1, 4}, (game_action_t){16, 73, 28, 65535, 112, 1, 4}, (game_action_t){16, 59, 25, 65535, 113, 1, 4}, (game_action_t){16, 74, 28, 65535, 65535, 1, 4}, (game_action_t){16, 72, 27, 65535, 115, 1, 4}, (game_action_t){16, 73, 28, 65535, 116, 1, 4}, (game_action_t){16, 59, 25, 65535, 117, 1, 4}, (game_action_t){16, 74, 28, 65535, 65535, 1, 4}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 65, 31, 120, 65535, 1, 1}, (game_action_t){16, 67, 25, 121, 65535, 1, 1}, (game_action_t){16, 4, 17, 122, 65535, 1, 1}, (game_action_t){16, 4, 17, 123, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 65, 31, 125, 65535, 1, 1}, (game_action_t){16, 67, 25, 126, 65535, 1, 1}, (game_action_t){16, 4, 17, 127, 65535, 1, 1}, (game_action_t){16, 4, 17, 128, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 77, 38, 130, 65535, 1, 1}, (game_action_t){16, 67, 25, 131, 65535, 1, 1}, (game_action_t){16, 4, 17, 132, 65535, 1, 1}, (game_action_t){16, 4, 17, 133, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 79, 26, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 80, 37, 137, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 80, 37, 139, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 80, 37, 141, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 80, 37, 143, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 8, 0, 65535, 65535, 1, 1}, (game_action_t){2, 9, 0, 65535, 65535, 1, 1}, (game_action_t){16, 87, 22, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 88, 32, 149, 65535, 1, 1}, (game_action_t){16, 89, 24, 150, 65535, 1, 1}, (game_action_t){16, 67, 25, 151, 65535, 1, 1}, (game_action_t){16, 4, 17, 152, 65535, 1, 1}, (game_action_t){16, 4, 17, 153, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 91, 39, 155, 65535, 1, 1}, (game_action_t){16, 67, 25, 156, 65535, 1, 1}, (game_action_t){16, 4, 17, 157, 65535, 1, 1}, (game_action_t){16, 4, 17, 158, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 93, 32, 160, 65535, 1, 1}, (game_action_t){16, 89, 24, 161, 65535, 1, 1}, (game_action_t){16, 67, 25, 162, 65535, 1, 1}, (game_action_t){16, 4, 17, 163, 65535, 1, 1}, (game_action_t){16, 4, 17, 164, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 95, 29, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 167, 65535, 1, 1}, (game_action_t){16, 10, 36, 168, 65535, 1, 1}, (game_action_t){16, 11, 26, 169, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 87, 22, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 91, 39, 173, 65535, 1, 1}, (game_action_t){16, 67, 25, 174, 65535, 1, 1}, (game_action_t){16, 4, 17, 175, 65535, 1, 1}, (game_action_t){16, 4, 17, 176, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 65535, 1, 1}, (game_action_t){16, 98, 25, 179, 65535, 1, 1}, (game_action_t){16, 67, 25, 180, 65535, 1, 1}, (game_action_t){16, 4, 17, 181, 65535, 1, 1}, (game_action_t){16, 4, 17, 182, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 100, 37, 184, 65535, 1, 1}, (game_action_t){16, 102, 27, 185, 65535, 1, 1}, (game_action_t){16, 4, 17, 186, 65535, 1, 1}, (game_action_t){16, 4, 17, 187, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 95, 29, 189, 65535, 1, 1}, (game_action_t){2, 10, 0, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 191, 65535, 1, 1}, (game_action_t){16, 10, 36, 192, 65535, 1, 1}, (game_action_t){16, 11, 26, 193, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 91, 39, 196, 65535, 1, 1}, (game_action_t){16, 67, 25, 197, 65535, 1, 1}, (game_action_t){16, 4, 17, 198, 65535, 1, 1}, (game_action_t){16, 4, 17, 199, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 65535, 1, 1}, (game_action_t){16, 98, 25, 202, 65535, 1, 1}, (game_action_t){16, 67, 25, 203, 65535, 1, 1}, (game_action_t){16, 4, 17, 204, 65535, 1, 1}, (game_action_t){16, 4, 17, 205, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 100, 37, 207, 65535, 1, 1}, (game_action_t){16, 102, 27, 208, 65535, 1, 1}, (game_action_t){16, 4, 17, 209, 65535, 1, 1}, (game_action_t){16, 4, 17, 210, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 95, 29, 65535, 65535, 1, 1}, (game_action_t){16, 8, 28, 213, 65535, 1, 1}, (game_action_t){16, 10, 36, 214, 65535, 1, 1}, (game_action_t){16, 11, 26, 215, 65535, 1, 1}, (game_action_t){2, 11, 0, 65535, 65535, 1, 1}, (game_action_t){16, 104, 29, 217, 65535, 1, 1}, (game_action_t){16, 105, 20, 218, 65535, 1, 1}, (game_action_t){16, 106, 28, 219, 65535, 1, 1}, (game_action_t){16, 107, 32, 220, 65535, 1, 1}, (game_action_t){16, 108, 34, 221, 65535, 1, 1}, (game_action_t){16, 109, 21, 222, 65535, 1, 1}, (game_action_t){16, 4, 17, 223, 65535, 1, 1}, (game_action_t){16, 109, 21, 224, 65535, 1, 1}, (game_action_t){16, 4, 17, 225, 65535, 1, 1}, (game_action_t){16, 109, 21, 226, 65535, 1, 1}, (game_action_t){16, 4, 17, 227, 65535, 1, 1}, (game_action_t){16, 110, 26, 228, 65535, 1, 1}, (game_action_t){16, 111, 38, 229, 65535, 1, 1}, (game_action_t){16, 112, 21, 230, 65535, 1, 1}, (game_action_t){16, 113, 33, 231, 65535, 1, 1}, (game_action_t){16, 114, 36, 232, 65535, 1, 1}, (game_action_t){16, 115, 20, 233, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 116, 28, 65535, 235, 1, 3}, (game_action_t){16, 117, 24, 65535, 236, 1, 3}, (game_action_t){16, 118, 37, 65535, 65535, 1, 3}, (game_action_t){16, 119, 25, 240, 238, 1, 3}, (game_action_t){16, 120, 26, 240, 239, 1, 3}, (game_action_t){16, 121, 26, 240, 65535, 1, 3}, (game_action_t){16, 122, 27, 244, 241, 1, 4}, (game_action_t){16, 123, 32, 244, 242, 1, 4}, (game_action_t){16, 124, 34, 244, 243, 1, 4}, (game_action_t){16, 125, 31, 244, 65535, 1, 4}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){100, 0, 0, 246, 65535, 1, 1}, (game_action_t){18, 127, 26, 247, 65535, 1, 1}, (game_action_t){16, 128, 39, 248, 65535, 1, 1}, (game_action_t){18, 129, 40, 249, 65535, 1, 1}, (game_action_t){16, 130, 37, 250, 65535, 1, 1}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 131, 26, 254, 252, 1, 3}, (game_action_t){16, 133, 25, 254, 253, 1, 3}, (game_action_t){16, 134, 21, 254, 65535, 1, 3}, (game_action_t){16, 135, 34, 257, 255, 1, 3}, (game_action_t){16, 136, 38, 257, 256, 1, 3}, (game_action_t){16, 137, 36, 257, 65535, 1, 3}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 138, 38, 259, 65535, 1, 1}, (game_action_t){16, 140, 39, 260, 65535, 1, 1}, (game_action_t){16, 141, 38, 261, 65535, 1, 1}, (game_action_t){16, 142, 36, 262, 65535, 1, 1}, (game_action_t){16, 143, 33, 263, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 144, 27, 267, 265, 1, 3}, (game_action_t){16, 146, 34, 267, 266, 1, 3}, (game_action_t){16, 147, 35, 267, 65535, 1, 3}, (game_action_t){16, 148, 24, 270, 268, 1, 3}, (game_action_t){16, 149, 34, 270, 269, 1, 3}, (game_action_t){16, 150, 29, 270, 65535, 1, 3}, (game_action_t){16, 151, 30, 271, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 119, 25, 276, 273, 1, 4}, (game_action_t){16, 120, 26, 276, 274, 1, 4}, (game_action_t){16, 152, 30, 276, 275, 1, 4}, (game_action_t){16, 153, 23, 276, 65535, 1, 4}, (game_action_t){16, 122, 27, 280, 277, 1, 4}, (game_action_t){16, 154, 30, 280, 278, 1, 4}, (game_action_t){16, 124, 34, 280, 279, 1, 4}, (game_action_t){16, 125, 31, 280, 65535, 1, 4}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){16, 155, 23, 65535, 282, 1, 4}, (game_action_t){16, 156, 36, 65535, 283, 1, 4}, (game_action_t){16, 157, 30, 65535, 284, 1, 4}, (game_action_t){18, 158, 37, 65535, 65535, 1, 4}, (game_action_t){16, 159, 40, 286, 65535, 1, 1}, (game_action_t){16, 161, 38, 287, 65535, 1, 1}, (game_action_t){16, 162, 34, 288, 65535, 1, 1}, (game_action_t){16, 163, 36, 289, 65535, 1, 1}, (game_action_t){16, 164, 96, 290, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 165, 32, 292, 65535, 1, 1}, (game_action_t){16, 167, 33, 293, 294, 1, 3}, (game_action_t){16, 168, 24, 297, 65535, 1, 1}, (game_action_t){16, 169, 39, 297, 295, 1, 3}, (game_action_t){16, 170, 39, 296, 65535, 1, 3}, (game_action_t){16, 171, 20, 297, 65535, 1, 1}, (game_action_t){16, 172, 38, 298, 65535, 1, 1}, (game_action_t){17, 173, 25, 299, 65535, 1, 1}, (game_action_t){16, 174, 39, 300, 65535, 1, 1}, (game_action_t){16, 175, 24, 301, 65535, 1, 1}, (game_action_t){16, 176, 96, 302, 65535, 1, 1}, (game_action_t){16, 177, 96, 303, 65535, 1, 1}, (game_action_t){2, 14, 0, 65535, 65535, 1, 1}, (game_action_t){16, 178, 35, 306, 305, 1, 2}, (game_action_t){16, 180, 28, 306, 65535, 1, 2}, (game_action_t){16, 181, 30, 309, 307, 1, 3}, (game_action_t){16, 182, 33, 309, 308, 1, 3}, (game_action_t){16, 183, 38, 309, 65535, 1, 3}, (game_action_t){16, 184, 32, 310, 65535, 1, 1}, (game_action_t){16, 185, 30, 312, 311, 1, 2}, (game_action_t){16, 186, 25, 312, 65535, 1, 2}, (game_action_t){16, 187, 96, 313, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 188, 29, 65535, 315, 1, 4}, (game_action_t){16, 189, 34, 65535, 316, 1, 4}, (game_action_t){16, 190, 37, 65535, 317, 1, 4}, (game_action_t){18, 158, 37, 65535, 65535, 1, 4}, (game_action_t){16, 191, 31, 319, 65535, 1, 1}, (game_action_t){16, 4, 17, 320, 65535, 1, 1}, (game_action_t){2, 43, 0, 65535, 65535, 1, 1}, (game_action_t){16, 192, 26, 324, 322, 1, 3}, (game_action_t){16, 194, 23, 324, 323, 1, 3}, (game_action_t){16, 195, 35, 324, 65535, 1, 3}, (game_action_t){16, 196, 32, 329, 325, 1, 4}, (game_action_t){16, 197, 31, 329, 326, 1, 4}, (game_action_t){16, 198, 31, 327, 328, 1, 4}, (game_action_t){16, 199, 26, 329, 65535, 1, 1}, (game_action_t){16, 200, 39, 329, 65535, 1, 4}, (game_action_t){16, 201, 33, 330, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 202, 21, 332, 65535, 1, 1}, (game_action_t){16, 204, 33, 333, 65535, 1, 1}, (game_action_t){16, 205, 40, 334, 65535, 1, 1}, (game_action_t){16, 206, 20, 335, 65535, 1, 1}, (game_action_t){16, 207, 36, 336, 65535, 1, 1}, (game_action_t){16, 208, 37, 337, 65535, 1, 1}, (game_action_t){16, 209, 39, 338, 65535, 1, 1}, (game_action_t){16, 4, 17, 339, 65535, 1, 1}, (game_action_t){16, 210, 33, 340, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 211, 35, 342, 65535, 1, 1}, (game_action_t){16, 213, 36, 343, 65535, 1, 1}, (game_action_t){16, 214, 37, 344, 65535, 1, 1}, (game_action_t){16, 4, 17, 345, 65535, 1, 1}, (game_action_t){16, 215, 24, 346, 65535, 1, 1}, (game_action_t){2, 15, 0, 65535, 65535, 1, 1}, (game_action_t){16, 155, 23, 348, 65535, 1, 1}, (game_action_t){16, 217, 34, 349, 65535, 1, 1}, (game_action_t){16, 218, 22, 350, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){1, 0, 0, 352, 65535, 1, 1}, (game_action_t){16, 219, 22, 353, 65535, 1, 1}, (game_action_t){16, 220, 27, 354, 65535, 1, 1}, (game_action_t){16, 221, 27, 355, 65535, 1, 1}, (game_action_t){16, 222, 27, 356, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){18, 223, 39, 358, 65535, 1, 1}, (game_action_t){16, 224, 25, 359, 65535, 1, 1}, (game_action_t){16, 226, 37, 360, 65535, 1, 1}, (game_action_t){16, 227, 25, 361, 65535, 1, 1}, (game_action_t){16, 228, 21, 362, 65535, 1, 1}, (game_action_t){1, 1, 0, 363, 65535, 1, 1}, (game_action_t){16, 229, 23, 364, 65535, 1, 1}, (game_action_t){16, 4, 17, 365, 65535, 1, 1}, (game_action_t){16, 230, 37, 366, 65535, 1, 1}, (game_action_t){6, 0, 0, 367, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 231, 25, 369, 65535, 1, 1}, (game_action_t){16, 232, 34, 370, 65535, 1, 1}, (game_action_t){16, 233, 26, 371, 65535, 1, 1}, (game_action_t){16, 234, 35, 372, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 235, 34, 374, 65535, 1, 1}, (game_action_t){16, 236, 24, 375, 65535, 1, 1}, (game_action_t){16, 238, 38, 376, 65535, 1, 1}, (game_action_t){16, 239, 24, 377, 65535, 1, 1}, (game_action_t){16, 240, 38, 378, 65535, 1, 1}, (game_action_t){2, 17, 0, 65535, 65535, 1, 1}, (game_action_t){16, 241, 29, 380, 65535, 1, 1}, (game_action_t){16, 243, 32, 381, 65535, 1, 1}, (game_action_t){16, 244, 36, 382, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){16, 241, 29, 384, 65535, 1, 1}, (game_action_t){16, 243, 32, 385, 65535, 1, 1}, (game_action_t){16, 244, 36, 386, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){16, 245, 34, 388, 65535, 1, 1}, (game_action_t){16, 246, 33, 389, 65535, 1, 1}, (game_action_t){16, 247, 38, 390, 65535, 1, 1}, (game_action_t){16, 248, 19, 391, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 249, 38, 393, 65535, 1, 1}, (game_action_t){16, 251, 35, 394, 65535, 1, 1}, (game_action_t){16, 252, 37, 395, 65535, 1, 1}, (game_action_t){16, 253, 35, 65535, 65535, 1, 1}, (game_action_t){16, 254, 20, 397, 65535, 1, 1}, (game_action_t){16, 256, 34, 398, 65535, 1, 1}, (game_action_t){16, 257, 40, 399, 65535, 1, 1}, (game_action_t){16, 258, 26, 400, 65535, 1, 1}, (game_action_t){16, 259, 36, 65535, 65535, 1, 1}, (game_action_t){16, 260, 26, 402, 65535, 1, 1}, (game_action_t){16, 262, 30, 403, 65535, 1, 1}, (game_action_t){16, 263, 40, 404, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 264, 23, 409, 406, 1, 4}, (game_action_t){16, 265, 31, 409, 407, 1, 4}, (game_action_t){16, 266, 32, 409, 408, 1, 4}, (game_action_t){16, 267, 40, 409, 65535, 1, 4}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 411, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){16, 211, 35, 413, 65535, 1, 1}, (game_action_t){16, 213, 36, 414, 65535, 1, 1}, (game_action_t){16, 214, 37, 415, 65535, 1, 1}, (game_action_t){16, 4, 17, 416, 65535, 1, 1}, (game_action_t){16, 215, 24, 417, 65535, 1, 1}, (game_action_t){2, 15, 0, 65535, 65535, 1, 1}, (game_action_t){16, 268, 29, 419, 65535, 1, 1}, (game_action_t){16, 270, 37, 420, 65535, 1, 1}, (game_action_t){16, 271, 26, 421, 65535, 1, 1}, (game_action_t){16, 272, 28, 422, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 273, 32, 424, 65535, 1, 1}, (game_action_t){16, 275, 25, 430, 425, 1, 5}, (game_action_t){16, 276, 32, 430, 426, 1, 5}, (game_action_t){16, 277, 39, 427, 428, 1, 5}, (game_action_t){16, 278, 31, 430, 65535, 1, 1}, (game_action_t){16, 279, 31, 430, 429, 1, 5}, (game_action_t){16, 280, 31, 430, 65535, 1, 5}, (game_action_t){16, 281, 21, 433, 431, 1, 3}, (game_action_t){16, 282, 24, 433, 432, 1, 3}, (game_action_t){16, 283, 28, 433, 65535, 1, 3}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){16, 284, 32, 435, 65535, 1, 1}, (game_action_t){16, 286, 34, 436, 65535, 1, 1}, (game_action_t){16, 287, 22, 437, 65535, 1, 1}, (game_action_t){2, 21, 0, 65535, 65535, 1, 1}, (game_action_t){1, 3, 0, 439, 65535, 1, 1}, (game_action_t){18, 288, 30, 65535, 65535, 1, 1}, (game_action_t){16, 289, 30, 441, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 290, 20, 65535, 443, 1, 4}, (game_action_t){18, 288, 30, 65535, 444, 1, 4}, (game_action_t){16, 291, 26, 65535, 445, 1, 4}, (game_action_t){16, 292, 27, 65535, 65535, 1, 4}, (game_action_t){16, 293, 32, 447, 65535, 1, 1}, (game_action_t){16, 295, 29, 448, 65535, 1, 1}, (game_action_t){16, 296, 40, 449, 65535, 1, 1}, (game_action_t){16, 297, 33, 450, 65535, 1, 1}, (game_action_t){16, 298, 35, 451, 65535, 1, 1}, (game_action_t){16, 299, 38, 452, 65535, 1, 1}, (game_action_t){6, 0, 0, 453, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 300, 40, 455, 65535, 1, 1}, (game_action_t){16, 301, 29, 456, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 302, 35, 458, 65535, 1, 1}, (game_action_t){16, 304, 34, 460, 459, 1, 2}, (game_action_t){16, 305, 34, 460, 65535, 1, 2}, (game_action_t){16, 4, 17, 461, 65535, 1, 1}, (game_action_t){16, 306, 38, 462, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 307, 27, 464, 65535, 1, 1}, (game_action_t){16, 4, 17, 465, 65535, 1, 1}, (game_action_t){16, 309, 30, 466, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 310, 33, 468, 65535, 1, 1}, (game_action_t){16, 4, 17, 469, 65535, 1, 1}, (game_action_t){2, 22, 0, 65535, 65535, 1, 1}, (game_action_t){16, 312, 27, 471, 65535, 1, 1}, (game_action_t){16, 313, 24, 472, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){18, 314, 26, 474, 65535, 1, 1}, (game_action_t){16, 315, 36, 475, 65535, 1, 1}, (game_action_t){2, 23, 0, 65535, 65535, 1, 1}, (game_action_t){16, 316, 35, 477, 65535, 1, 1}, (game_action_t){16, 318, 40, 478, 65535, 1, 1}, (game_action_t){2, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 319, 30, 480, 65535, 1, 1}, (game_action_t){16, 321, 36, 482, 481, 1, 2}, (game_action_t){16, 322, 33, 482, 65535, 1, 2}, (game_action_t){16, 323, 29, 484, 483, 1, 2}, (game_action_t){16, 324, 31, 484, 65535, 1, 2}, (game_action_t){16, 325, 30, 485, 65535, 1, 1}, (game_action_t){16, 4, 17, 65535, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){16, 327, 31, 488, 65535, 1, 1}, (game_action_t){16, 329, 38, 489, 65535, 1, 1}, (game_action_t){16, 330, 40, 490, 65535, 1, 1}, (game_action_t){16, 331, 30, 491, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 332, 39, 493, 65535, 1, 1}, (game_action_t){16, 334, 37, 494, 65535, 1, 1}, (game_action_t){16, 335, 40, 495, 65535, 1, 1}, (game_action_t){16, 336, 37, 496, 65535, 1, 1}, (game_action_t){16, 337, 20, 497, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 338, 40, 499, 65535, 1, 1}, (game_action_t){16, 340, 37, 500, 65535, 1, 1}, (game_action_t){16, 341, 22, 501, 65535, 1, 1}, (game_action_t){16, 342, 39, 502, 65535, 1, 1}, (game_action_t){16, 343, 38, 503, 65535, 1, 1}, (game_action_t){16, 344, 38, 504, 65535, 1, 1}, (game_action_t){16, 345, 19, 505, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 346, 35, 507, 65535, 1, 1}, (game_action_t){16, 348, 40, 508, 65535, 1, 1}, (game_action_t){16, 349, 33, 509, 65535, 1, 1}, (game_action_t){16, 350, 38, 510, 65535, 1, 1}, (game_action_t){16, 351, 21, 511, 65535, 1, 1}, (game_action_t){16, 352, 37, 512, 65535, 1, 1}, (game_action_t){16, 353, 21, 513, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){2, 40, 0, 65535, 65535, 1, 1}, (game_action_t){2, 44, 0, 65535, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 517, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 356, 22, 523, 520, 1, 4}, (game_action_t){16, 358, 28, 523, 521, 1, 4}, (game_action_t){16, 359, 32, 523, 522, 1, 4}, (game_action_t){16, 360, 23, 523, 65535, 1, 4}, (game_action_t){16, 361, 27, 525, 524, 1, 2}, (game_action_t){16, 362, 27, 525, 65535, 1, 2}, (game_action_t){19, 363, 34, 65535, 65535, 1, 1}, (game_action_t){16, 364, 24, 528, 527, 1, 2}, (game_action_t){16, 366, 19, 528, 65535, 1, 2}, (game_action_t){16, 367, 38, 529, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 533, 4, 8}, (game_action_t){16, 368, 160, 532, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 370, 21, 65535, 534, 1, 8}, (game_action_t){16, 371, 40, 535, 536, 1, 8}, (game_action_t){16, 372, 20, 65535, 65535, 1, 1}, (game_action_t){16, 373, 29, 65535, 537, 1, 8}, (game_action_t){16, 374, 37, 538, 65535, 1, 8}, (game_action_t){16, 375, 35, 539, 65535, 1, 1}, (game_action_t){16, 376, 24, 65535, 65535, 1, 1}, (game_action_t){16, 377, 27, 542, 541, 1, 2}, (game_action_t){16, 379, 35, 542, 65535, 1, 2}, (game_action_t){16, 380, 36, 543, 65535, 1, 1}, (game_action_t){16, 381, 32, 544, 65535, 1, 1}, (game_action_t){16, 233, 26, 545, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 382, 160, 547, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){2, 45, 0, 65535, 65535, 1, 1}, (game_action_t){2, 28, 0, 65535, 65535, 1, 1}, (game_action_t){16, 356, 22, 554, 551, 1, 4}, (game_action_t){16, 358, 28, 554, 552, 1, 4}, (game_action_t){16, 359, 32, 554, 553, 1, 4}, (game_action_t){16, 360, 23, 554, 65535, 1, 4}, (game_action_t){16, 361, 27, 556, 555, 1, 2}, (game_action_t){16, 362, 27, 556, 65535, 1, 2}, (game_action_t){19, 363, 34, 65535, 65535, 1, 1}, (game_action_t){16, 364, 24, 559, 558, 1, 2}, (game_action_t){16, 366, 19, 559, 65535, 1, 2}, (game_action_t){16, 367, 38, 560, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 562, 4, 7}, (game_action_t){16, 384, 32, 563, 564, 1, 7}, (game_action_t){16, 385, 25, 65535, 65535, 1, 1}, (game_action_t){16, 386, 40, 65535, 565, 1, 7}, (game_action_t){16, 387, 35, 65535, 65535, 1, 7}, (game_action_t){16, 377, 27, 568, 567, 1, 2}, (game_action_t){16, 379, 35, 568, 65535, 1, 2}, (game_action_t){16, 380, 36, 569, 65535, 1, 1}, (game_action_t){16, 381, 32, 570, 65535, 1, 1}, (game_action_t){16, 233, 26, 571, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 388, 23, 573, 65535, 1, 1}, (game_action_t){16, 152, 30, 574, 65535, 1, 1}, (game_action_t){16, 390, 31, 575, 65535, 1, 1}, (game_action_t){2, 39, 0, 65535, 65535, 1, 1}, (game_action_t){2, 29, 0, 65535, 65535, 1, 1}, (game_action_t){2, 30, 0, 65535, 65535, 1, 1}, (game_action_t){16, 356, 22, 582, 579, 1, 4}, (game_action_t){16, 358, 28, 582, 580, 1, 4}, (game_action_t){16, 359, 32, 582, 581, 1, 4}, (game_action_t){16, 360, 23, 582, 65535, 1, 4}, (game_action_t){16, 361, 27, 584, 583, 1, 2}, (game_action_t){16, 362, 27, 584, 65535, 1, 2}, (game_action_t){19, 363, 34, 65535, 65535, 1, 1}, (game_action_t){16, 364, 24, 587, 586, 1, 2}, (game_action_t){16, 366, 19, 587, 65535, 1, 2}, (game_action_t){16, 367, 38, 588, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 590, 4, 7}, (game_action_t){16, 384, 32, 591, 592, 1, 7}, (game_action_t){16, 385, 25, 65535, 65535, 1, 1}, (game_action_t){16, 386, 40, 65535, 593, 1, 7}, (game_action_t){16, 387, 35, 65535, 65535, 1, 7}, (game_action_t){16, 377, 27, 596, 595, 1, 2}, (game_action_t){16, 379, 35, 596, 65535, 1, 2}, (game_action_t){16, 380, 36, 597, 65535, 1, 1}, (game_action_t){16, 381, 32, 598, 65535, 1, 1}, (game_action_t){16, 233, 26, 599, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 392, 28, 601, 65535, 1, 1}, (game_action_t){16, 393, 37, 602, 65535, 1, 1}, (game_action_t){16, 394, 40, 603, 65535, 1, 1}, (game_action_t){16, 395, 21, 604, 65535, 1, 1}, (game_action_t){16, 396, 40, 605, 65535, 1, 1}, (game_action_t){16, 397, 34, 606, 65535, 1, 1}, (game_action_t){16, 398, 37, 607, 65535, 1, 1}, (game_action_t){16, 399, 38, 608, 65535, 1, 1}, (game_action_t){6, 0, 0, 609, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 31, 0, 65535, 65535, 1, 1}, (game_action_t){2, 32, 0, 65535, 65535, 1, 1}, (game_action_t){16, 356, 22, 616, 613, 1, 4}, (game_action_t){16, 358, 28, 616, 614, 1, 4}, (game_action_t){16, 359, 32, 616, 615, 1, 4}, (game_action_t){16, 360, 23, 616, 65535, 1, 4}, (game_action_t){16, 361, 27, 618, 617, 1, 2}, (game_action_t){16, 362, 27, 618, 65535, 1, 2}, (game_action_t){19, 363, 34, 65535, 65535, 1, 1}, (game_action_t){16, 364, 24, 621, 620, 1, 2}, (game_action_t){16, 366, 19, 621, 65535, 1, 2}, (game_action_t){16, 367, 38, 622, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 624, 4, 7}, (game_action_t){16, 384, 32, 625, 626, 1, 7}, (game_action_t){16, 385, 25, 65535, 65535, 1, 1}, (game_action_t){16, 386, 40, 65535, 627, 1, 7}, (game_action_t){16, 387, 35, 65535, 65535, 1, 7}, (game_action_t){16, 377, 27, 630, 629, 1, 2}, (game_action_t){16, 379, 35, 630, 65535, 1, 2}, (game_action_t){16, 380, 36, 631, 65535, 1, 1}, (game_action_t){16, 381, 32, 632, 65535, 1, 1}, (game_action_t){16, 233, 26, 633, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 388, 23, 635, 65535, 1, 1}, (game_action_t){16, 152, 30, 636, 65535, 1, 1}, (game_action_t){16, 390, 31, 637, 65535, 1, 1}, (game_action_t){2, 39, 0, 65535, 65535, 1, 1}, (game_action_t){16, 401, 32, 645, 639, 1, 6}, (game_action_t){16, 402, 34, 640, 641, 1, 6}, (game_action_t){16, 403, 25, 645, 65535, 1, 1}, (game_action_t){16, 404, 38, 645, 642, 1, 6}, (game_action_t){17, 405, 35, 645, 643, 1, 6}, (game_action_t){16, 406, 35, 645, 644, 1, 6}, (game_action_t){17, 407, 31, 645, 65535, 1, 6}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 33, 0, 65535, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 356, 22, 652, 649, 1, 4}, (game_action_t){16, 358, 28, 652, 650, 1, 4}, (game_action_t){16, 359, 32, 652, 651, 1, 4}, (game_action_t){16, 360, 23, 652, 65535, 1, 4}, (game_action_t){16, 361, 27, 654, 653, 1, 2}, (game_action_t){16, 362, 27, 654, 65535, 1, 2}, (game_action_t){19, 363, 34, 65535, 65535, 1, 1}, (game_action_t){16, 364, 24, 657, 656, 1, 2}, (game_action_t){16, 366, 19, 657, 65535, 1, 2}, (game_action_t){16, 367, 38, 658, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 660, 4, 7}, (game_action_t){16, 384, 32, 661, 662, 1, 7}, (game_action_t){16, 385, 25, 65535, 65535, 1, 1}, (game_action_t){16, 386, 40, 65535, 663, 1, 7}, (game_action_t){16, 387, 35, 65535, 65535, 1, 7}, (game_action_t){16, 377, 27, 666, 665, 1, 2}, (game_action_t){16, 379, 35, 666, 65535, 1, 2}, (game_action_t){16, 380, 36, 667, 65535, 1, 1}, (game_action_t){16, 381, 32, 668, 65535, 1, 1}, (game_action_t){16, 233, 26, 669, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 409, 38, 671, 65535, 1, 1}, (game_action_t){16, 410, 36, 672, 65535, 1, 1}, (game_action_t){16, 411, 28, 673, 65535, 1, 1}, (game_action_t){6, 0, 0, 674, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 388, 96, 65535, 65535, 1, 1}, (game_action_t){2, 34, 0, 65535, 65535, 1, 1}, (game_action_t){2, 35, 0, 65535, 65535, 1, 1}, (game_action_t){17, 414, 38, 65535, 679, 1, 4}, (game_action_t){16, 416, 28, 65535, 680, 1, 4}, (game_action_t){16, 417, 29, 65535, 681, 1, 4}, (game_action_t){16, 418, 37, 65535, 65535, 1, 4}, (game_action_t){16, 419, 39, 683, 65535, 1, 1}, (game_action_t){16, 420, 25, 684, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 421, 40, 65535, 65535, 1, 1}, (game_action_t){2, 48, 0, 65535, 65535, 1, 1}, (game_action_t){16, 422, 160, 688, 65535, 1, 1}, (game_action_t){2, 34, 0, 65535, 65535, 1, 1}, (game_action_t){2, 37, 0, 65535, 65535, 1, 1}, (game_action_t){2, 38, 0, 65535, 65535, 1, 1}, (game_action_t){16, 426, 39, 692, 65535, 1, 1}, (game_action_t){16, 427, 31, 693, 65535, 1, 1}, (game_action_t){16, 105, 20, 694, 65535, 1, 1}, (game_action_t){16, 428, 29, 695, 65535, 1, 1}, (game_action_t){16, 429, 28, 696, 65535, 1, 1}, (game_action_t){16, 430, 36, 697, 65535, 1, 1}, (game_action_t){16, 431, 36, 698, 65535, 1, 1}, (game_action_t){16, 432, 22, 699, 65535, 1, 1}, (game_action_t){16, 4, 17, 700, 65535, 1, 1}, (game_action_t){16, 433, 37, 701, 65535, 1, 1}, (game_action_t){16, 434, 21, 702, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 435, 30, 704, 65535, 1, 1}, (game_action_t){16, 436, 33, 705, 65535, 1, 1}, (game_action_t){16, 437, 38, 706, 65535, 1, 1}, (game_action_t){16, 438, 25, 707, 65535, 1, 1}, (game_action_t){16, 4, 17, 708, 65535, 1, 1}, (game_action_t){16, 430, 36, 709, 65535, 1, 1}, (game_action_t){16, 439, 26, 710, 65535, 1, 1}, (game_action_t){16, 440, 39, 711, 65535, 1, 1}, (game_action_t){16, 441, 25, 712, 65535, 1, 1}, (game_action_t){16, 442, 40, 713, 65535, 1, 1}, (game_action_t){16, 443, 34, 714, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 419, 39, 716, 65535, 1, 1}, (game_action_t){16, 420, 25, 717, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 444, 25, 65535, 65535, 1, 1}, (game_action_t){16, 445, 36, 720, 65535, 1, 1}, (game_action_t){16, 447, 38, 721, 65535, 1, 1}, (game_action_t){16, 448, 38, 722, 65535, 1, 1}, (game_action_t){16, 449, 25, 723, 65535, 1, 1}, (game_action_t){16, 450, 39, 724, 65535, 1, 1}, (game_action_t){16, 451, 39, 725, 65535, 1, 1}, (game_action_t){16, 452, 38, 726, 65535, 1, 1}, (game_action_t){16, 453, 40, 727, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 454, 29, 729, 65535, 1, 1}, (game_action_t){16, 456, 37, 730, 65535, 1, 1}, (game_action_t){16, 450, 39, 731, 65535, 1, 1}, (game_action_t){16, 451, 39, 732, 65535, 1, 1}, (game_action_t){16, 452, 38, 733, 65535, 1, 1}, (game_action_t){16, 453, 40, 734, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 457, 27, 736, 65535, 1, 1}, (game_action_t){16, 458, 39, 737, 65535, 1, 1}, (game_action_t){16, 459, 28, 738, 65535, 1, 1}, (game_action_t){16, 460, 37, 739, 65535, 1, 1}, (game_action_t){16, 461, 38, 740, 65535, 1, 1}, (game_action_t){16, 462, 40, 741, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){100, 2, 0, 743, 65535, 1, 1}, (game_action_t){16, 463, 31, 65535, 65535, 1, 1}, (game_action_t){16, 464, 28, 745, 65535, 1, 1}, (game_action_t){16, 466, 38, 746, 65535, 1, 1}, (game_action_t){16, 395, 21, 747, 65535, 1, 1}, (game_action_t){16, 467, 38, 748, 65535, 1, 1}, (game_action_t){16, 468, 37, 749, 65535, 1, 1}, (game_action_t){16, 469, 27, 750, 65535, 1, 1}, (game_action_t){16, 470, 39, 751, 65535, 1, 1}, (game_action_t){16, 471, 31, 752, 65535, 1, 1}, (game_action_t){6, 0, 0, 753, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 472, 33, 755, 65535, 1, 1}, (game_action_t){16, 473, 32, 756, 65535, 1, 1}, (game_action_t){16, 474, 38, 757, 65535, 1, 1}, (game_action_t){16, 475, 30, 758, 65535, 1, 1}, (game_action_t){6, 0, 0, 759, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 476, 30, 761, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 477, 26, 764, 763, 1, 2}, (game_action_t){16, 478, 29, 764, 65535, 1, 2}, (game_action_t){19, 479, 35, 65535, 765, 1, 3}, (game_action_t){19, 480, 28, 65535, 766, 1, 3}, (game_action_t){19, 481, 30, 65535, 65535, 1, 3}, (game_action_t){16, 482, 21, 770, 768, 1, 3}, (game_action_t){16, 484, 24, 770, 769, 1, 3}, (game_action_t){16, 485, 31, 770, 65535, 1, 3}, (game_action_t){16, 4, 96, 65535, 65535, 1, 1}, (game_action_t){16, 486, 25, 772, 65535, 1, 1}, (game_action_t){16, 487, 33, 773, 65535, 1, 1}, (game_action_t){16, 488, 30, 774, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 489, 21, 779, 776, 1, 3}, (game_action_t){16, 490, 40, 779, 777, 1, 3}, (game_action_t){16, 491, 40, 778, 65535, 1, 3}, (game_action_t){16, 492, 27, 779, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 493, 31, 781, 65535, 1, 1}, (game_action_t){16, 494, 34, 782, 65535, 1, 1}, (game_action_t){16, 495, 36, 783, 65535, 1, 1}, (game_action_t){16, 496, 23, 784, 65535, 1, 1}, (game_action_t){16, 497, 29, 785, 65535, 1, 1}, (game_action_t){16, 498, 37, 786, 65535, 1, 1}, (game_action_t){16, 4, 17, 787, 65535, 1, 1}, (game_action_t){16, 499, 38, 788, 65535, 1, 1}, (game_action_t){6, 0, 0, 789, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 500, 32, 791, 65535, 1, 1}, (game_action_t){16, 501, 35, 792, 65535, 1, 1}, (game_action_t){16, 502, 26, 793, 65535, 1, 1}, (game_action_t){16, 503, 40, 794, 65535, 1, 1}, (game_action_t){16, 504, 38, 795, 65535, 1, 1}, (game_action_t){16, 505, 28, 796, 65535, 1, 1}, (game_action_t){6, 0, 0, 797, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}};

game_state_t all_states[] = {(game_state_t){.entry_series_id=0, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=4}}, .input_series={(game_user_in_t){.text_addr=5, .result_action_id=9},(game_user_in_t){.text_addr=6, .result_action_id=10},(game_user_in_t){.text_addr=7, .result_action_id=11},(game_user_in_t){.text_addr=9, .result_action_id=15},(game_user_in_t){.text_addr=12, .result_action_id=19},(game_user_in_t){.text_addr=13, .result_action_id=20}}, .other_series={}}, (game_state_t){.entry_series_id=21, .timer_series_len=0, .input_series_len=0, .other_series_len=2, .timer_series={}, .input_series={}, .other_series={(game_other_in_t){.type_id=2, .result_action_id=23},(game_other_in_t){.type_id=3, .result_action_id=27}}}, (game_state_t){.entry_series_id=34, .timer_series_len=1, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=36}}, .input_series={(game_user_in_t){.text_addr=26, .result_action_id=37},(game_user_in_t){.text_addr=43, .result_action_id=45},(game_user_in_t){.text_addr=46, .result_action_id=56},(game_user_in_t){.text_addr=55, .result_action_id=62}}, .other_series={}}, (game_state_t){.entry_series_id=65, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=69},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=73}}, .input_series={(game_user_in_t){.text_addr=9, .result_action_id=74},(game_user_in_t){.text_addr=60, .result_action_id=78},(game_user_in_t){.text_addr=61, .result_action_id=79}}, .other_series={}}, (game_state_t){.entry_series_id=80, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=83},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=86}}, .input_series={(game_user_in_t){.text_addr=66, .result_action_id=87}}, .other_series={}}, (game_state_t){.entry_series_id=92, .timer_series_len=2, .input_series_len=2, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=96},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=100}}, .input_series={(game_user_in_t){.text_addr=71, .result_action_id=101},(game_user_in_t){.text_addr=7, .result_action_id=106}}, .other_series={}}, (game_state_t){.entry_series_id=110, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=114},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=118}}, .input_series={(game_user_in_t){.text_addr=75, .result_action_id=119},(game_user_in_t){.text_addr=76, .result_action_id=124},(game_user_in_t){.text_addr=78, .result_action_id=129}}, .other_series={}}, (game_state_t){.entry_series_id=134, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=135}}, .input_series={(game_user_in_t){.text_addr=81, .result_action_id=136},(game_user_in_t){.text_addr=82, .result_action_id=138},(game_user_in_t){.text_addr=83, .result_action_id=140},(game_user_in_t){.text_addr=84, .result_action_id=142},(game_user_in_t){.text_addr=85, .result_action_id=144},(game_user_in_t){.text_addr=86, .result_action_id=145}}, .other_series={}}, (game_state_t){.entry_series_id=146, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=147}}, .input_series={(game_user_in_t){.text_addr=90, .result_action_id=148},(game_user_in_t){.text_addr=92, .result_action_id=154},(game_user_in_t){.text_addr=94, .result_action_id=159},(game_user_in_t){.text_addr=96, .result_action_id=165},(game_user_in_t){.text_addr=9, .result_action_id=166}}, .other_series={}}, (game_state_t){.entry_series_id=170, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=171}}, .input_series={(game_user_in_t){.text_addr=92, .result_action_id=172},(game_user_in_t){.text_addr=97, .result_action_id=177},(game_user_in_t){.text_addr=99, .result_action_id=178},(game_user_in_t){.text_addr=101, .result_action_id=183},(game_user_in_t){.text_addr=103, .result_action_id=188},(game_user_in_t){.text_addr=9, .result_action_id=190}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=194}}, .input_series={(game_user_in_t){.text_addr=92, .result_action_id=195},(game_user_in_t){.text_addr=97, .result_action_id=200},(game_user_in_t){.text_addr=99, .result_action_id=201},(game_user_in_t){.text_addr=101, .result_action_id=206},(game_user_in_t){.text_addr=103, .result_action_id=211},(game_user_in_t){.text_addr=9, .result_action_id=212}}, .other_series={}}, (game_state_t){.entry_series_id=216, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=234},(game_timer_t){.duration=1152, .recurring=0, .result_action_id=237}}, .input_series={(game_user_in_t){.text_addr=126, .result_action_id=245},(game_user_in_t){.text_addr=132, .result_action_id=251},(game_user_in_t){.text_addr=139, .result_action_id=258},(game_user_in_t){.text_addr=145, .result_action_id=264}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=1152, .recurring=0, .result_action_id=272},(game_timer_t){.duration=256, .recurring=0, .result_action_id=281}}, .input_series={(game_user_in_t){.text_addr=160, .result_action_id=285},(game_user_in_t){.text_addr=166, .result_action_id=291},(game_user_in_t){.text_addr=179, .result_action_id=304}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=480, .recurring=0, .result_action_id=314},(game_timer_t){.duration=38400, .recurring=0, .result_action_id=318}}, .input_series={(game_user_in_t){.text_addr=193, .result_action_id=321},(game_user_in_t){.text_addr=203, .result_action_id=331},(game_user_in_t){.text_addr=212, .result_action_id=341},(game_user_in_t){.text_addr=216, .result_action_id=347}}, .other_series={}}, (game_state_t){.entry_series_id=351, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=225, .result_action_id=357}}, .other_series={}}, (game_state_t){.entry_series_id=368, .timer_series_len=0, .input_series_len=2, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=237, .result_action_id=373},(game_user_in_t){.text_addr=242, .result_action_id=379}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=242, .result_action_id=383}}, .other_series={}}, (game_state_t){.entry_series_id=387, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=250, .result_action_id=392},(game_user_in_t){.text_addr=255, .result_action_id=396},(game_user_in_t){.text_addr=261, .result_action_id=401}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=405},(game_timer_t){.duration=90240, .recurring=0, .result_action_id=410}}, .input_series={(game_user_in_t){.text_addr=212, .result_action_id=412},(game_user_in_t){.text_addr=269, .result_action_id=418},(game_user_in_t){.text_addr=274, .result_action_id=423},(game_user_in_t){.text_addr=285, .result_action_id=434}}, .other_series={}}, (game_state_t){.entry_series_id=438, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=960, .recurring=0, .result_action_id=440},(game_timer_t){.duration=64, .recurring=0, .result_action_id=442}}, .input_series={(game_user_in_t){.text_addr=294, .result_action_id=446}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=454}}, .input_series={(game_user_in_t){.text_addr=303, .result_action_id=457},(game_user_in_t){.text_addr=308, .result_action_id=463},(game_user_in_t){.text_addr=311, .result_action_id=467}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=0, .other_series_len=2, .timer_series={}, .input_series={}, .other_series={(game_other_in_t){.type_id=0, .result_action_id=470},(game_other_in_t){.type_id=1, .result_action_id=473}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=317, .result_action_id=476}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=320, .result_action_id=479},(game_user_in_t){.text_addr=326, .result_action_id=486},(game_user_in_t){.text_addr=328, .result_action_id=487}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=333, .result_action_id=492},(game_user_in_t){.text_addr=339, .result_action_id=498},(game_user_in_t){.text_addr=347, .result_action_id=506}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=9600, .recurring=0, .result_action_id=516},(game_timer_t){.duration=21120, .recurring=0, .result_action_id=518}}, .input_series={(game_user_in_t){.text_addr=354, .result_action_id=514},(game_user_in_t){.text_addr=355, .result_action_id=515},(game_user_in_t){.text_addr=357, .result_action_id=519},(game_user_in_t){.text_addr=365, .result_action_id=526},(game_user_in_t){.text_addr=369, .result_action_id=530},(game_user_in_t){.text_addr=378, .result_action_id=540}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=549}}, .input_series={(game_user_in_t){.text_addr=383, .result_action_id=548},(game_user_in_t){.text_addr=357, .result_action_id=550},(game_user_in_t){.text_addr=365, .result_action_id=557},(game_user_in_t){.text_addr=369, .result_action_id=561},(game_user_in_t){.text_addr=378, .result_action_id=566},(game_user_in_t){.text_addr=389, .result_action_id=572}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=577}}, .input_series={(game_user_in_t){.text_addr=391, .result_action_id=576},(game_user_in_t){.text_addr=357, .result_action_id=578},(game_user_in_t){.text_addr=365, .result_action_id=585},(game_user_in_t){.text_addr=369, .result_action_id=589},(game_user_in_t){.text_addr=378, .result_action_id=594}}, .other_series={}}, (game_state_t){.entry_series_id=600, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=611}}, .input_series={(game_user_in_t){.text_addr=400, .result_action_id=610},(game_user_in_t){.text_addr=357, .result_action_id=612},(game_user_in_t){.text_addr=365, .result_action_id=619},(game_user_in_t){.text_addr=369, .result_action_id=623},(game_user_in_t){.text_addr=378, .result_action_id=628},(game_user_in_t){.text_addr=389, .result_action_id=634}}, .other_series={}}, (game_state_t){.entry_series_id=638, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=647}}, .input_series={(game_user_in_t){.text_addr=408, .result_action_id=646},(game_user_in_t){.text_addr=357, .result_action_id=648},(game_user_in_t){.text_addr=365, .result_action_id=655},(game_user_in_t){.text_addr=369, .result_action_id=659},(game_user_in_t){.text_addr=378, .result_action_id=664}}, .other_series={}}, (game_state_t){.entry_series_id=675, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=670}}, .input_series={(game_user_in_t){.text_addr=412, .result_action_id=676},(game_user_in_t){.text_addr=413, .result_action_id=677},(game_user_in_t){.text_addr=415, .result_action_id=678}}, .other_series={}}, (game_state_t){.entry_series_id=685, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=682}}, .input_series={(game_user_in_t){.text_addr=423, .result_action_id=686},(game_user_in_t){.text_addr=424, .result_action_id=689},(game_user_in_t){.text_addr=425, .result_action_id=690}}, .other_series={}}, (game_state_t){.entry_series_id=691, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=703, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=718, .timer_series_len=1, .input_series_len=2, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=715}}, .input_series={(game_user_in_t){.text_addr=446, .result_action_id=719},(game_user_in_t){.text_addr=455, .result_action_id=728}}, .other_series={}}, (game_state_t){.entry_series_id=735, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=742, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=465, .result_action_id=744}}, .other_series={}}, (game_state_t){.entry_series_id=754, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=762, .timer_series_len=1, .input_series_len=0, .other_series_len=0, .timer_series={(game_timer_t){.duration=320, .recurring=0, .result_action_id=760}}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=771}}, .input_series={(game_user_in_t){.text_addr=483, .result_action_id=767}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=483, .result_action_id=775}}, .other_series={}}, (game_state_t){.entry_series_id=780, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=790, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=531, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=546, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=687, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}};

#define SPECIAL_BADGESNEARBY0 0
#define SPECIAL_BADGESNEARBYSOME 1
#define SPECIAL_NAME_NOT_FOUND 2
#define SPECIAL_NAME_FOUND 3
#define STATE_ID_FIRSTBOOT 0
#define STATE_ID_CUSTOMSTATENAMESEARCH 1
#define STATE_ID_FIRSTBOOTCONFUSED 2
#define STATE_ID_FIRSTBOOTHELLO 3
#define STATE_ID_FIRSTBOOTBROKEN 4
#define STATE_ID_FIRSTBOOTHELLOSPEAK 5
#define STATE_ID_FIRSTBOOTHELLOLEARN 6
#define STATE_ID_FIRSTBOOTSETSTATE 7
#define STATE_ID_SETCOUNTRY 8
#define STATE_ID_SETLANGUAGE 9
#define STATE_ID_INITIALIZELANGUAGE 10
#define STATE_ID_WELCOMETOSKIPPY 11
#define STATE_ID_WHOAREYOU 12
#define STATE_ID_STARTINGTOLEARN 13
#define STATE_ID_WEHAVENAMES 14
#define STATE_ID_LIGHTSON 15
#define STATE_ID_SKIPPYSTORY1 16
#define STATE_ID_SKIPPYSTORY2 17
#define STATE_ID_SKIPPYSTORY3 18
#define STATE_ID_WEHAVEAMISSION 19
#define STATE_ID_LOCALDISCOVERYNOTWO 20
#define STATE_ID_DOASCAN 21
#define STATE_ID_LOCALDEVICESCAN 22
#define STATE_ID_FIRSTCONTACT 23
#define STATE_ID_WHATDIDYOUFIND 24
#define STATE_ID_WHATDIDYOUFINDDETAILS 25
#define STATE_ID_CRACKINGTHEFILE1 26
#define STATE_ID_CRACKINGTHEFILE2 27
#define STATE_ID_CRACKINGTHEFILE3 28
#define STATE_ID_DOYOUHAVEALLTHEFILE 29
#define STATE_ID_CRACKINGTHEFILE4 30
#define STATE_ID_HEYSKIPPY 31
#define STATE_ID_CRACKINGTHEFILE5 32
#define STATE_ID_WHATISTHEFILE 33
#define STATE_ID_IKNOWQUEERCON 34
#define STATE_ID_DONTKNOWQUEERCON 35
#define STATE_ID_JUST_JOINED 36
#define STATE_ID_FROMTHEBEGINNING 37
#define STATE_ID_FAIRLYWELL 38
#define STATE_ID_FILELIGHTSON 39
#define STATE_ID_WHYTHEFILEISENCRYPTED 40
#define STATE_ID_CONNECTEDTONETWORK 41
#define STATE_ID_ANGRYSLEEP 42
#define STATE_ID_SLEEP 43
#define STATE_ID_DOYOUHAVESPEAKERS 44
#define STATE_ID_HOWOLDAREYOU 45
#define STATE_ID_CUSTOMSTATESTATUS 46
#define STATE_ID_COLORFINDER 47
#define STATE_ID_JUSTJOINED 48
#define OTHER_ACTION_CUSTOMSTATEUSERNAME 0
#define OTHER_ACTION_NAMESEARCH 1
#define OTHER_ACTION_TURN_ON_THE_LIGHTS_TO_REPRESENT_FILE_STATE 2
#define CLOSABLE_STATES 8



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

/// Check if a special event should fire, and fire it, returning 1 if it fired.
uint8_t game_process_special() {
    for (uint8_t i=0; i<current_state->other_series_len; i++) {
        if (current_state->other_series[i].type_id == SPECIAL_BADGESNEARBY0 &&
                badges_nearby==0) {
            if (start_action_series(current_state->other_series[i].result_action_id))
                return 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_BADGESNEARBYSOME &&
                badges_nearby>0) {
            if (start_action_series(current_state->other_series[i].result_action_id))
                return 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_NAME_FOUND &&
                s_game_checkname_success) {
            // Joy!
            s_game_checkname_success = 0;
            if (start_action_series(current_state->other_series[i].result_action_id))
                return 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_NAME_NOT_FOUND &&
                !s_game_checkname_success) {
            // No joy.
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

    memcpy(&loaded_state, &all_states[state_id], sizeof(game_state_t));

    current_state = &loaded_state;
    current_state_id = state_id;

    start_action_series(current_state->entry_series_id);
}

extern uint16_t gd_starting_id;

void game_begin() {
    all_animations[0] = anim_lsw;
    all_animations[1] = anim_lwf;
    all_animations[2] = anim_spinblue;
    all_animations[3] = anim_whitediscovery;

    // TODO: stored state
    game_set_state(STATE_ID_WEHAVEAMISSION);
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

/// Render bottom screen for the current state and value of `text_selection`.
void draw_text_selection() {
    lcd111_cursor_pos(LCD_BTM, 0);
    lcd111_put_char(LCD_BTM, 0xBB); // This is &raquo;
    if (text_selection) {
        lcd111_cursor_type(LCD_BTM, LCD111_CURSOR_NONE);
        if (strlen(all_text[current_state->input_series[text_selection-1].text_addr]) > 21) {
            lcd111_put_text_pad(
                    LCD_BTM,
                    all_text[current_state->input_series[text_selection-1].text_addr],
                    23
            );
        } else {
            lcd111_put_text_pad(
                    LCD_BTM,
                    all_text[current_state->input_series[text_selection-1].text_addr],
                    21
            );
            if (next_input_id() != text_selection) {
                lcd111_put_text_pad(LCD_BTM, "\x1E\x1F", 2);
            } else {
                lcd111_put_text_pad(LCD_BTM, " \x17", 2);
            }
        }
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
extern uint16_t gd_next_id;
extern uint16_t gd_starting_id;

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
        if (action->detail == OTHER_ACTION_CUSTOMSTATEUSERNAME) {
            textentry_begin(badge_conf.person_name, 10, 1, 1);
        } else if (action->detail == OTHER_ACTION_NAMESEARCH) {
            gd_starting_id = GAME_NULL;
            gd_next_id = GAME_NULL;
            qc15_mode = QC15_MODE_GAME_CHECKNAME;
            // IPC GET NEXT ID from ffff (any)
            while (!ipc_tx_op_buf(IPC_MSG_ID_NEXT, &gd_starting_id, 2));
            textentry_begin(game_name_buffer, 10, 0, 0);
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
