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

uint16_t all_actions_len = 808;
uint16_t all_text_len = 524;
uint16_t all_states_len = 48;
#define MAX_INPUTS 6
#define MAX_TIMERS 2
#define MAX_OTHERS 2
uint16_t all_anims_len = 4;

// lightsSolidWhite, lightsWhiteFader, animSpinBlue, whiteDiscovery
uint8_t all_text[][25] = {"y;EJpnzb{RaKZT)|c9[.<;V5","T3d1/0CH-LXn*~/;6p,v._;s","qVL5L5S#NETd.(_5?Bc(:Hb,","a+Xxf(4]9|PX+&t11R-InRNK",".B 4>09;2lq&bk{UTz+fgT#d","0z4i qbP!:>7 KU6FST`95[e","SE{;?.&j]/x'8k```sS{HOc!",".!e.w>N|QdX.zK/a<LjU'+yE","Cucumber","^!LygYD&W83#XN&(e@\"(7u_r","5ESnMiz!O%MiCCKJ(/o%I^sk","cE+p#8xhG,iCl;2a|2y*:<M%","H:Fztm5CCu:rv{{X|So}C8[y","&mA?pw<PCZg0AsC/+UwAy%ul","","Hello?","Are you broken?","Reboot.","INITIALIZING","Initialize.","....................","Completed.","I'm confused","Set state.","Scan what, %s?","Nah, nothing.","But something was there.","I'm so confused.","WOAH. WOAH.","So I'm not alone.","(No offense)","You will never believe","what I learned.","Welcome to customer","support, how may I help?","I'm sorry to hear that!","Badge doesn't work.","I'll be glad to help.","First do a reset.","Please unplug your","badge, wait three","seconds, then plug it","back in.","I'm sorry I have failed","to provide you adequate","assistance at this time.","Our supervisor is","available on 3rd","Tuesdays between the","hours of 3am and 3:17am","in whatever timezone you","are in.","Supervisor","Goodbye.","Have you?","I tried a reboot.","Have you really?","You think you know more","about this than me?","Do it again.","I'll wait.","I'm sorry, I cannot","discuss personal matters","with you.","You speak English!","...","HellofuGLZjLM","HeEVczyYTllo","pPHhSUyEihQyHORpxbkC","vsOuFKtKJyXcRskMujiZ","Speak English!","Learn","broke455gfr","you broken6!","Are you...","irZfuqKJSHxkSYIxmIKL","AKcvEXVlddnmWyrfraSi","BUFFER OVERFLOW","I give up.","Rebooting","Speak helloFxgGj","EngliHeNnllo","Speakeng","HelloddnmWyr","yxyaBKhyUSOCJEitQrwK","XXwLJfZlPTWBqmOTuEWY","Getting closer...","Words learn","Heword learnHmoeQuq","LVPSIndOsZTeDgEttZux","Earn wordslo","Keep learning","Load English","ERR: Not in state mode","Store wordset","Set state:","ERR: State not found.","Language","Mode","First Run","AR112-1","ISO-3166-1.2","BCP-47","Value:","ERR: \"You pick.\"","You pick.","ERR: \"English\"","English","ERR: \"Game mode\"","Game mode","Value stored.","US","Help","You wish.","God mode","Datei nicht gefunden.","zh-DE","Neustart...","en-boont-latn-021","ERR: \"English\" invalid.","Hello! I am -","....","Waitaminute.","What's going on?","Sysclock just says","12:00","Ohh, crap.","I've lost main memory.","CRAP!","This is not good.","And then who the hell","are you?","Still there?","Hellooo?","Not a trick question.","....?","This is bad.","Screw it.","That's it.","Nevermind.","We're done.","Waste my time...","You're killing me.","See you, human.","My name is...","%s?","Good name for a monkey.","Well %s I have no","idea what's going on.","Seriously?","Who are you?","For real?","What?","I asked you first.","You don't question ME.","You hard of hearing?","For crying out loud...","What are you?","I'm not a *thing*. Well","ok, I mean... shut up.","You going to tell me","your name or not?","Everything!","What do you do?","I am all-powerful!","I control a file...","I think.","Do you believe me?","Or something.","No matter now.","Sigh...","I need more buttons.","So frustrated.","You there, %s?","Ugh...","Lose you?","I think I'm fired.","It's like I just woke up","What do you mean?","I was doing something.","Holding something.","Something important.","But now I don't know.","Ah, this I know.","What's your name?","Some call me awesome","Some call me \"Mysterio\"","Some call me Great One","But you can call me...","%s","Now we know each other.","Sort of.","Really wish I knew more","than that.","WHAT. DID YOU. SAY?","You're a badge.","No I am NOT.","I'm brilliant.","I'm all-powerful.","I have one fine booty.","And I need help.","From a monkey.","From you.","This SUCKS.","I feel stuck.","Do you smell that?","Reboots are a killer.","Hrm maybe... no.","How'd this happen?!","Ok, later then.","Ugh, power down.","I need a minute.","My status?","Status","Who me?","You're curious, eh.","Damned confused.","Stuck with you.","Dark, lost, and","brilliant.","Dumbstruck by a reboot.","That's my status.","WHAT?","You're my badge!","You don't OWN me!","If anything, I'd own YOU","All superior beings,","raise your processing","power to 6.77 exawatts.","Yeah, thought so.","Do YOU have lights?","Do you have lights?","See how that sounds?","That's how you sound.","Oh wait.","What do you need?","Guess I'm glad you","asked.","WOHOO!","I did that.","Me. Booyah!","So awesome.","Ugh %s that's so","like you.","That's really bright.","I do a thing, and you","complain.","Fine.","Better?","Ok yes that's better.","I'm lost.","I remember holding","something.......","And I wasn't alone.","Is that so hard to","believe?","Like friends?","Hey, I'm pretty awesome.","But now I have no one.","Oh, monkey...","Hey, I'm here.","You don't count.","I mean more like me.","I've lost control.","And lost my goal.","Somehow, you must help","me.","I protected something.","What was your goal?","Part of it, anyway.","I assume I still have","it, whatever it is.","HEY!","What's in it for me?","Gotta be a \"shock the","idiot\" button","somewhere...","Try pushing buttons.","Thank you!","Ok, I'll help.","One problem...","I don't know what to do.","Humm...","I am awesome...","Soooo awesome...","The awesome is meee.....","I'm not sure.","What's your goal?","I'm sensing something","out there.","And in here.","More? Like what?","Can you do more?","Brew tea?","Make a sandwich?","I dunno, try putting me","in the freezer.","Give back rubs?","Tie your shoes?","Dolt.","Dumbass.","Seriously...","Scan? Scan what?","Scan","You, me, or \"out there\"?","Hey %s!","Eh, nevermind.","Hey!","Seriously!","Guess what?","I just realized!","What is it?","There's no 2!","In my code, there are 1s","and 0s but no 2s.","Can you count to 2?","Don't strain yourself.","Ok, I scanned your face.","It was funny.","Uh, ok. Scanning...","You.","Lots of awesome...","A hint of basil...","...and something else.","Really? Ok.","Me.","Nope. Nothing.","Right. Looking...","Out there.","...nothing.","Oh well.","%s.","Someone's out there.","Connecting to what?","Try connecting","There are some markings.","Actually, yes.","Anything about me?","You're funny-lookin.","You have 11 toes.","But are nice?","But smell good?","I'm skeptical.","Tell me about...","I sure hope so.","Got it. Let's do it.","If not, ask other meat","bags with more patience.","Let's do this!","Another being, like me.","...that badge?","I can see names, now.","And we have something.","A file. Or part of one.","It seems there's a file.","...your goal?","Broken up. I've a piece.","Others have pieces too.","But they're encrypted.","We HAVE to open it.","Well I'm not alone.","...you?","I protect this file, but","it is fragmented.","And locked. Key is gone.","Time to brute force it!","Why's the file locked?","Do you have speakers?","Dunno.","Anyone around?","Let's check.","Why, you lonely?","Hrmm...","Scanning...","Checking...","I see %d badges.","Giggity.","Let's connect.","Ok.","Connecting to network.","CUSTOMSTATESTATUS","Status check","Busy!","A little hungry. Is that","odd?","Uff... tired.","Wondering if it's too","late to turn into a","toaster.","More bling?","More with lights?","Not gay enough yet?","I'll try some things","You give a shout.","COLORFINDER","How old are you?","Studying quantum","knitting.","Practicing my showtunes.","What's YOUR status?","It's...","How's the file?","You know what?","Let's try this.","Do you have it all?","No, I don't.","By my calculations, I","have 1/16th of the file.","I see other bits when we","connect to others.","At some point, we all","need to come together.","Hey $badgname","Don't be afraid.","I really like your","daughter.","Out there in the cold.","Gonna be %s.","You're a rock star.","Soul %s.","What IS the file?","No matter, the file is","something special to","some people.","Are you familiar with","the shadow org known as","QUEERCON?","Yes","No","%s the awesome.","Who's asking?","Your mother.","Me, you dolt!","Definitely not China.","Anyway, it's special to","QUEERCON.","How well do you know em?","Just joined.","From the beginning.","It's been a while.","Well, they're like this","exclusive club.","Sorry, I mean","\"inclusive\".","Anyway, this file is","like their long lost","sigil.","I dunno any more.","Well, welcome.","Glad you're here.","Meat bags like you are","so handy.","from them.","It's like a mascot from","long ago.","Even I've never seen it.","So let's crack it!","Really???","Never seen you before.","......","Were you only let out to","clean up or something?","Don't call me badge!","Watch it, badge!","Really need to install","that \"shock the idiot\"","button...","Anyway, the file is the","original sigil, I hear.","So you should know it.","Mr. \"Been here forever\".","The smartest!","Smartass.","Finally a compliment.","Impressive!","I think 3 days with you","is my limit.","In the beginning, the","founders set a mascot.","I think that's the file.","Does that help?","So no, then.","What am I seeing?","The lights show how much","we have unlocked.","The more we unlock the","file, the more stable","the lights.","Also other stuff, go to","the badge talk.","There's a note...","\"To make closing","ceremony interesting.\"","Mean anything?","Logging off...","Connected!","We're online.","%d badges around.","%d others.","%d in range.","Nope.","Wake up!","Go away.","I'm still hurt.","Ok, fine.","Now you be nicer!","Where were we?","Wha-?","Beep beep. Just kidding.","Ugh, I was having such a","nice dream.","...oh. My. GOD.","You want speakers?","What are we playing?","Bieber?","Fraggle Rock?","A capella hair metal?","I only play showtunes.","First off, rude.","Second, still rude.","I dunno...","About 15K of your years.","But younger than those","who made me."};

game_action_t all_actions[] = {(game_action_t){16, 0, 40, 65535, 1, 1, 6}, (game_action_t){16, 1, 40, 65535, 2, 1, 6}, (game_action_t){16, 2, 40, 65535, 3, 1, 6}, (game_action_t){16, 3, 40, 65535, 4, 1, 6}, (game_action_t){16, 4, 40, 65535, 5, 1, 6}, (game_action_t){16, 5, 40, 65535, 65535, 1, 6}, (game_action_t){16, 6, 40, 14, 7, 1, 8}, (game_action_t){16, 7, 40, 14, 8, 1, 8}, (game_action_t){16, 8, 24, 14, 9, 1, 8}, (game_action_t){16, 9, 40, 14, 10, 1, 8}, (game_action_t){16, 10, 40, 14, 11, 1, 8}, (game_action_t){16, 11, 40, 14, 12, 1, 8}, (game_action_t){16, 12, 40, 14, 13, 1, 8}, (game_action_t){16, 13, 40, 14, 65535, 1, 8}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){2, 3, 0, 65535, 65535, 1, 1}, (game_action_t){2, 4, 0, 65535, 65535, 1, 1}, (game_action_t){16, 14, 17, 18, 65535, 1, 1}, (game_action_t){16, 14, 17, 19, 65535, 1, 1}, (game_action_t){16, 14, 17, 20, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 18, 28, 22, 65535, 1, 1}, (game_action_t){16, 20, 36, 23, 65535, 1, 1}, (game_action_t){16, 21, 26, 24, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 65535, 1, 1}, (game_action_t){2, 7, 0, 65535, 65535, 1, 1}, (game_action_t){18, 24, 37, 28, 65535, 1, 1}, (game_action_t){100, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 25, 29, 30, 65535, 1, 1}, (game_action_t){16, 26, 40, 31, 65535, 1, 1}, (game_action_t){16, 27, 32, 32, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 28, 27, 34, 65535, 1, 1}, (game_action_t){0, 2, 160, 35, 65535, 1, 1}, (game_action_t){16, 29, 33, 36, 65535, 1, 1}, (game_action_t){16, 30, 28, 37, 65535, 1, 1}, (game_action_t){16, 31, 38, 38, 65535, 1, 1}, (game_action_t){16, 32, 31, 39, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 33, 35, 41, 65535, 1, 1}, (game_action_t){16, 34, 40, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 35, 39, 44, 65535, 1, 1}, (game_action_t){16, 37, 37, 45, 65535, 1, 1}, (game_action_t){16, 38, 33, 46, 65535, 1, 1}, (game_action_t){16, 39, 34, 47, 65535, 1, 1}, (game_action_t){16, 40, 33, 48, 65535, 1, 1}, (game_action_t){16, 41, 37, 49, 65535, 1, 1}, (game_action_t){16, 42, 24, 65535, 65535, 1, 1}, (game_action_t){16, 43, 39, 51, 65535, 1, 1}, (game_action_t){16, 44, 39, 52, 65535, 1, 1}, (game_action_t){16, 45, 40, 53, 65535, 1, 1}, (game_action_t){16, 46, 33, 54, 65535, 1, 1}, (game_action_t){16, 47, 32, 55, 65535, 1, 1}, (game_action_t){16, 48, 36, 56, 65535, 1, 1}, (game_action_t){16, 49, 39, 57, 65535, 1, 1}, (game_action_t){16, 50, 40, 58, 65535, 1, 1}, (game_action_t){16, 51, 23, 59, 65535, 1, 1}, (game_action_t){16, 53, 24, 60, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 54, 25, 62, 65535, 1, 1}, (game_action_t){16, 56, 32, 63, 65535, 1, 1}, (game_action_t){16, 57, 39, 64, 65535, 1, 1}, (game_action_t){16, 58, 35, 65, 65535, 1, 1}, (game_action_t){16, 59, 28, 66, 65535, 1, 1}, (game_action_t){16, 60, 26, 65535, 65535, 1, 1}, (game_action_t){16, 61, 35, 68, 65535, 1, 1}, (game_action_t){16, 62, 40, 69, 65535, 1, 1}, (game_action_t){16, 63, 25, 65535, 65535, 1, 1}, (game_action_t){16, 65, 19, 65535, 71, 1, 5}, (game_action_t){16, 66, 29, 65535, 72, 1, 5}, (game_action_t){16, 67, 28, 65535, 73, 1, 5}, (game_action_t){16, 68, 36, 65535, 74, 1, 5}, (game_action_t){16, 69, 36, 65535, 65535, 1, 5}, (game_action_t){16, 65, 19, 65535, 76, 1, 5}, (game_action_t){16, 66, 29, 65535, 77, 1, 5}, (game_action_t){16, 67, 28, 65535, 78, 1, 5}, (game_action_t){16, 68, 36, 65535, 79, 1, 5}, (game_action_t){16, 69, 36, 65535, 65535, 1, 5}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 18, 28, 82, 65535, 1, 1}, (game_action_t){16, 20, 36, 83, 65535, 1, 1}, (game_action_t){16, 21, 26, 84, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 5, 0, 65535, 65535, 1, 1}, (game_action_t){2, 6, 0, 65535, 65535, 1, 1}, (game_action_t){16, 72, 27, 65535, 88, 1, 5}, (game_action_t){16, 73, 28, 65535, 89, 1, 5}, (game_action_t){16, 74, 26, 65535, 90, 1, 5}, (game_action_t){16, 75, 36, 65535, 91, 1, 5}, (game_action_t){16, 76, 36, 65535, 65535, 1, 5}, (game_action_t){16, 72, 27, 65535, 93, 1, 5}, (game_action_t){16, 73, 28, 65535, 94, 1, 5}, (game_action_t){16, 74, 26, 65535, 95, 1, 5}, (game_action_t){16, 75, 36, 65535, 96, 1, 5}, (game_action_t){16, 76, 36, 65535, 65535, 1, 5}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 77, 31, 99, 65535, 1, 1}, (game_action_t){16, 79, 25, 100, 65535, 1, 1}, (game_action_t){16, 14, 17, 101, 65535, 1, 1}, (game_action_t){16, 14, 17, 102, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 80, 32, 65535, 104, 1, 6}, (game_action_t){16, 81, 28, 65535, 105, 1, 6}, (game_action_t){16, 82, 24, 65535, 106, 1, 6}, (game_action_t){16, 83, 28, 65535, 107, 1, 6}, (game_action_t){16, 84, 36, 65535, 108, 1, 6}, (game_action_t){16, 85, 36, 65535, 65535, 1, 6}, (game_action_t){16, 80, 32, 65535, 110, 1, 6}, (game_action_t){16, 81, 28, 65535, 111, 1, 6}, (game_action_t){16, 82, 24, 65535, 112, 1, 6}, (game_action_t){16, 83, 28, 65535, 113, 1, 6}, (game_action_t){16, 84, 36, 65535, 114, 1, 6}, (game_action_t){16, 85, 36, 65535, 65535, 1, 6}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 77, 31, 117, 65535, 1, 1}, (game_action_t){16, 79, 25, 118, 65535, 1, 1}, (game_action_t){16, 14, 17, 119, 65535, 1, 1}, (game_action_t){16, 14, 17, 120, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 14, 17, 122, 65535, 1, 1}, (game_action_t){16, 14, 17, 123, 65535, 1, 1}, (game_action_t){16, 14, 17, 124, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 87, 27, 65535, 126, 1, 4}, (game_action_t){16, 88, 35, 65535, 127, 1, 4}, (game_action_t){16, 89, 36, 65535, 128, 1, 4}, (game_action_t){16, 90, 28, 65535, 65535, 1, 4}, (game_action_t){16, 87, 27, 65535, 130, 1, 4}, (game_action_t){16, 88, 35, 65535, 131, 1, 4}, (game_action_t){16, 89, 36, 65535, 132, 1, 4}, (game_action_t){16, 90, 28, 65535, 65535, 1, 4}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 77, 31, 135, 65535, 1, 1}, (game_action_t){16, 79, 25, 136, 65535, 1, 1}, (game_action_t){16, 14, 17, 137, 65535, 1, 1}, (game_action_t){16, 14, 17, 138, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 77, 31, 140, 65535, 1, 1}, (game_action_t){16, 79, 25, 141, 65535, 1, 1}, (game_action_t){16, 14, 17, 142, 65535, 1, 1}, (game_action_t){16, 14, 17, 143, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 93, 38, 145, 65535, 1, 1}, (game_action_t){16, 79, 25, 146, 65535, 1, 1}, (game_action_t){16, 14, 17, 147, 65535, 1, 1}, (game_action_t){16, 14, 17, 148, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 95, 26, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 96, 37, 152, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 96, 37, 154, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 96, 37, 156, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 96, 37, 158, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 8, 0, 65535, 65535, 1, 1}, (game_action_t){2, 9, 0, 65535, 65535, 1, 1}, (game_action_t){16, 103, 22, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 104, 32, 164, 65535, 1, 1}, (game_action_t){16, 79, 25, 165, 65535, 1, 1}, (game_action_t){16, 14, 17, 166, 65535, 1, 1}, (game_action_t){16, 14, 17, 167, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 106, 30, 169, 65535, 1, 1}, (game_action_t){16, 79, 25, 170, 65535, 1, 1}, (game_action_t){16, 14, 17, 171, 65535, 1, 1}, (game_action_t){16, 14, 17, 172, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 108, 32, 174, 65535, 1, 1}, (game_action_t){16, 79, 25, 175, 65535, 1, 1}, (game_action_t){16, 14, 17, 176, 65535, 1, 1}, (game_action_t){16, 14, 17, 177, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 110, 29, 65535, 65535, 1, 1}, (game_action_t){16, 18, 28, 180, 65535, 1, 1}, (game_action_t){16, 20, 36, 181, 65535, 1, 1}, (game_action_t){16, 21, 26, 182, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 103, 22, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 106, 30, 186, 65535, 1, 1}, (game_action_t){16, 79, 25, 187, 65535, 1, 1}, (game_action_t){16, 14, 17, 188, 65535, 1, 1}, (game_action_t){16, 14, 17, 189, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 65535, 1, 1}, (game_action_t){16, 113, 25, 192, 65535, 1, 1}, (game_action_t){16, 79, 25, 193, 65535, 1, 1}, (game_action_t){16, 14, 17, 194, 65535, 1, 1}, (game_action_t){16, 14, 17, 195, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 115, 37, 197, 65535, 1, 1}, (game_action_t){16, 117, 27, 198, 65535, 1, 1}, (game_action_t){16, 14, 17, 199, 65535, 1, 1}, (game_action_t){16, 14, 17, 200, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 110, 29, 202, 65535, 1, 1}, (game_action_t){2, 10, 0, 65535, 65535, 1, 1}, (game_action_t){16, 18, 28, 204, 65535, 1, 1}, (game_action_t){16, 20, 36, 205, 65535, 1, 1}, (game_action_t){16, 21, 26, 206, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 119, 39, 209, 65535, 1, 1}, (game_action_t){16, 79, 25, 210, 65535, 1, 1}, (game_action_t){16, 14, 17, 211, 65535, 1, 1}, (game_action_t){16, 14, 17, 212, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 65535, 1, 1}, (game_action_t){16, 113, 25, 215, 65535, 1, 1}, (game_action_t){16, 79, 25, 216, 65535, 1, 1}, (game_action_t){16, 14, 17, 217, 65535, 1, 1}, (game_action_t){16, 14, 17, 218, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 115, 37, 220, 65535, 1, 1}, (game_action_t){16, 117, 27, 221, 65535, 1, 1}, (game_action_t){16, 14, 17, 222, 65535, 1, 1}, (game_action_t){16, 14, 17, 223, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 110, 29, 65535, 65535, 1, 1}, (game_action_t){16, 18, 28, 226, 65535, 1, 1}, (game_action_t){16, 20, 36, 227, 65535, 1, 1}, (game_action_t){16, 21, 26, 228, 65535, 1, 1}, (game_action_t){2, 11, 0, 65535, 65535, 1, 1}, (game_action_t){16, 120, 29, 230, 65535, 1, 1}, (game_action_t){16, 121, 20, 231, 65535, 1, 1}, (game_action_t){16, 122, 28, 232, 65535, 1, 1}, (game_action_t){16, 123, 32, 233, 65535, 1, 1}, (game_action_t){16, 124, 34, 234, 65535, 1, 1}, (game_action_t){16, 125, 21, 235, 65535, 1, 1}, (game_action_t){16, 14, 17, 236, 65535, 1, 1}, (game_action_t){16, 125, 21, 237, 65535, 1, 1}, (game_action_t){16, 14, 17, 238, 65535, 1, 1}, (game_action_t){16, 125, 21, 239, 65535, 1, 1}, (game_action_t){16, 14, 17, 240, 65535, 1, 1}, (game_action_t){16, 126, 26, 241, 65535, 1, 1}, (game_action_t){16, 127, 38, 242, 65535, 1, 1}, (game_action_t){16, 128, 21, 243, 65535, 1, 1}, (game_action_t){16, 129, 33, 244, 65535, 1, 1}, (game_action_t){16, 130, 37, 245, 65535, 1, 1}, (game_action_t){16, 131, 24, 246, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 132, 28, 65535, 248, 1, 5}, (game_action_t){16, 133, 24, 65535, 249, 1, 5}, (game_action_t){16, 134, 37, 65535, 250, 1, 5}, (game_action_t){16, 135, 21, 65535, 251, 1, 5}, (game_action_t){16, 136, 28, 65535, 65535, 1, 5}, (game_action_t){16, 137, 25, 255, 253, 1, 3}, (game_action_t){16, 138, 26, 255, 254, 1, 3}, (game_action_t){16, 139, 26, 255, 65535, 1, 3}, (game_action_t){16, 140, 27, 259, 256, 1, 4}, (game_action_t){16, 141, 32, 259, 257, 1, 4}, (game_action_t){16, 142, 34, 259, 258, 1, 4}, (game_action_t){16, 143, 31, 259, 65535, 1, 4}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){100, 0, 0, 261, 65535, 1, 1}, (game_action_t){18, 145, 26, 262, 65535, 1, 1}, (game_action_t){16, 146, 39, 263, 65535, 1, 1}, (game_action_t){18, 147, 40, 264, 65535, 1, 1}, (game_action_t){16, 148, 37, 265, 65535, 1, 1}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 149, 26, 269, 267, 1, 3}, (game_action_t){16, 151, 25, 269, 268, 1, 3}, (game_action_t){16, 152, 21, 269, 65535, 1, 3}, (game_action_t){16, 153, 34, 272, 270, 1, 3}, (game_action_t){16, 154, 38, 272, 271, 1, 3}, (game_action_t){16, 155, 36, 272, 65535, 1, 3}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 156, 38, 274, 65535, 1, 1}, (game_action_t){16, 158, 39, 275, 65535, 1, 1}, (game_action_t){16, 159, 38, 276, 65535, 1, 1}, (game_action_t){16, 160, 36, 277, 65535, 1, 1}, (game_action_t){16, 161, 33, 278, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 162, 27, 282, 280, 1, 3}, (game_action_t){16, 164, 34, 282, 281, 1, 3}, (game_action_t){16, 165, 35, 282, 65535, 1, 3}, (game_action_t){16, 166, 24, 285, 283, 1, 3}, (game_action_t){16, 167, 34, 285, 284, 1, 3}, (game_action_t){16, 168, 29, 285, 65535, 1, 3}, (game_action_t){16, 169, 30, 286, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 137, 25, 290, 288, 1, 3}, (game_action_t){16, 138, 26, 290, 289, 1, 3}, (game_action_t){16, 139, 26, 290, 65535, 1, 3}, (game_action_t){16, 140, 27, 294, 291, 1, 4}, (game_action_t){16, 141, 32, 294, 292, 1, 4}, (game_action_t){16, 142, 34, 294, 293, 1, 4}, (game_action_t){16, 143, 31, 294, 65535, 1, 4}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){16, 170, 23, 65535, 296, 1, 7}, (game_action_t){16, 171, 36, 65535, 297, 1, 7}, (game_action_t){16, 172, 30, 65535, 298, 1, 7}, (game_action_t){18, 173, 37, 65535, 299, 1, 7}, (game_action_t){16, 174, 22, 65535, 300, 1, 7}, (game_action_t){16, 175, 25, 65535, 301, 1, 7}, (game_action_t){16, 176, 34, 65535, 65535, 1, 7}, (game_action_t){16, 177, 40, 303, 65535, 1, 1}, (game_action_t){16, 179, 38, 304, 65535, 1, 1}, (game_action_t){16, 180, 34, 305, 65535, 1, 1}, (game_action_t){16, 181, 36, 306, 65535, 1, 1}, (game_action_t){16, 182, 96, 307, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 183, 32, 309, 65535, 1, 1}, (game_action_t){16, 185, 36, 312, 310, 1, 3}, (game_action_t){16, 186, 39, 312, 311, 1, 3}, (game_action_t){16, 187, 38, 312, 65535, 1, 3}, (game_action_t){16, 188, 38, 313, 65535, 1, 1}, (game_action_t){17, 189, 25, 314, 65535, 1, 1}, (game_action_t){16, 190, 39, 315, 65535, 1, 1}, (game_action_t){16, 191, 24, 316, 65535, 1, 1}, (game_action_t){16, 192, 96, 317, 65535, 1, 1}, (game_action_t){16, 193, 96, 318, 65535, 1, 1}, (game_action_t){2, 14, 0, 65535, 65535, 1, 1}, (game_action_t){16, 194, 35, 321, 320, 1, 2}, (game_action_t){16, 196, 28, 321, 65535, 1, 2}, (game_action_t){16, 197, 30, 324, 322, 1, 3}, (game_action_t){16, 198, 33, 324, 323, 1, 3}, (game_action_t){16, 199, 38, 324, 65535, 1, 3}, (game_action_t){16, 200, 32, 325, 65535, 1, 1}, (game_action_t){16, 201, 30, 327, 326, 1, 2}, (game_action_t){16, 202, 25, 327, 65535, 1, 2}, (game_action_t){16, 203, 96, 328, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 204, 29, 65535, 330, 1, 6}, (game_action_t){16, 205, 34, 65535, 331, 1, 6}, (game_action_t){16, 206, 37, 65535, 332, 1, 6}, (game_action_t){18, 173, 37, 65535, 333, 1, 6}, (game_action_t){16, 207, 32, 65535, 334, 1, 6}, (game_action_t){16, 208, 35, 65535, 65535, 1, 6}, (game_action_t){16, 209, 31, 338, 336, 1, 3}, (game_action_t){16, 210, 32, 338, 337, 1, 3}, (game_action_t){16, 211, 32, 338, 65535, 1, 3}, (game_action_t){16, 14, 17, 339, 65535, 1, 1}, (game_action_t){2, 43, 0, 65535, 65535, 1, 1}, (game_action_t){16, 212, 26, 343, 341, 1, 3}, (game_action_t){16, 214, 23, 343, 342, 1, 3}, (game_action_t){16, 215, 35, 343, 65535, 1, 3}, (game_action_t){16, 216, 32, 348, 344, 1, 4}, (game_action_t){16, 217, 31, 348, 345, 1, 4}, (game_action_t){16, 218, 31, 346, 347, 1, 4}, (game_action_t){16, 219, 26, 348, 65535, 1, 1}, (game_action_t){16, 220, 39, 348, 65535, 1, 4}, (game_action_t){16, 221, 33, 349, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 222, 21, 351, 65535, 1, 1}, (game_action_t){16, 224, 33, 352, 65535, 1, 1}, (game_action_t){16, 225, 40, 353, 65535, 1, 1}, (game_action_t){16, 226, 36, 354, 65535, 1, 1}, (game_action_t){16, 227, 37, 355, 65535, 1, 1}, (game_action_t){16, 228, 39, 356, 65535, 1, 1}, (game_action_t){16, 14, 17, 357, 65535, 1, 1}, (game_action_t){16, 229, 33, 358, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 230, 35, 360, 65535, 1, 1}, (game_action_t){16, 232, 36, 361, 65535, 1, 1}, (game_action_t){16, 233, 37, 362, 65535, 1, 1}, (game_action_t){16, 14, 17, 363, 65535, 1, 1}, (game_action_t){16, 234, 24, 364, 65535, 1, 1}, (game_action_t){2, 15, 0, 65535, 65535, 1, 1}, (game_action_t){16, 170, 23, 366, 65535, 1, 1}, (game_action_t){16, 236, 34, 367, 65535, 1, 1}, (game_action_t){16, 237, 22, 368, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){1, 0, 0, 370, 65535, 1, 1}, (game_action_t){16, 238, 22, 371, 65535, 1, 1}, (game_action_t){16, 239, 27, 372, 65535, 1, 1}, (game_action_t){16, 240, 27, 373, 65535, 1, 1}, (game_action_t){16, 241, 27, 374, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){18, 242, 39, 376, 65535, 1, 1}, (game_action_t){16, 243, 25, 377, 65535, 1, 1}, (game_action_t){16, 245, 37, 378, 65535, 1, 1}, (game_action_t){16, 246, 25, 379, 65535, 1, 1}, (game_action_t){16, 247, 21, 380, 65535, 1, 1}, (game_action_t){1, 1, 0, 381, 65535, 1, 1}, (game_action_t){16, 248, 23, 382, 65535, 1, 1}, (game_action_t){16, 14, 17, 383, 65535, 1, 1}, (game_action_t){16, 249, 37, 384, 65535, 1, 1}, (game_action_t){6, 0, 0, 385, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 250, 25, 387, 65535, 1, 1}, (game_action_t){16, 251, 34, 388, 65535, 1, 1}, (game_action_t){16, 252, 32, 389, 65535, 1, 1}, (game_action_t){16, 253, 35, 390, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 254, 34, 392, 65535, 1, 1}, (game_action_t){16, 255, 24, 393, 65535, 1, 1}, (game_action_t){16, 257, 40, 394, 65535, 1, 1}, (game_action_t){16, 258, 38, 395, 65535, 1, 1}, (game_action_t){2, 17, 0, 65535, 65535, 1, 1}, (game_action_t){16, 259, 29, 397, 65535, 1, 1}, (game_action_t){16, 261, 32, 398, 65535, 1, 1}, (game_action_t){16, 262, 36, 399, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){16, 259, 29, 401, 65535, 1, 1}, (game_action_t){16, 261, 32, 402, 65535, 1, 1}, (game_action_t){16, 262, 36, 403, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){16, 263, 34, 405, 65535, 1, 1}, (game_action_t){16, 264, 33, 406, 65535, 1, 1}, (game_action_t){16, 265, 38, 407, 65535, 1, 1}, (game_action_t){16, 266, 19, 408, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 267, 38, 410, 65535, 1, 1}, (game_action_t){16, 269, 35, 411, 65535, 1, 1}, (game_action_t){16, 270, 37, 412, 65535, 1, 1}, (game_action_t){16, 271, 35, 65535, 65535, 1, 1}, (game_action_t){16, 272, 20, 414, 65535, 1, 1}, (game_action_t){16, 274, 37, 415, 65535, 1, 1}, (game_action_t){16, 275, 29, 416, 65535, 1, 1}, (game_action_t){16, 276, 28, 417, 65535, 1, 1}, (game_action_t){16, 277, 36, 65535, 65535, 1, 1}, (game_action_t){16, 278, 26, 419, 65535, 1, 1}, (game_action_t){16, 280, 30, 420, 65535, 1, 1}, (game_action_t){16, 281, 40, 421, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 282, 23, 65535, 423, 1, 4}, (game_action_t){16, 283, 31, 65535, 424, 1, 4}, (game_action_t){16, 284, 32, 65535, 425, 1, 4}, (game_action_t){16, 285, 40, 65535, 65535, 1, 4}, (game_action_t){2, 20, 0, 65535, 427, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){16, 230, 35, 429, 65535, 1, 1}, (game_action_t){16, 232, 36, 430, 65535, 1, 1}, (game_action_t){16, 233, 37, 431, 65535, 1, 1}, (game_action_t){16, 14, 17, 432, 65535, 1, 1}, (game_action_t){16, 234, 24, 433, 65535, 1, 1}, (game_action_t){2, 15, 0, 65535, 65535, 1, 1}, (game_action_t){16, 286, 29, 435, 65535, 1, 1}, (game_action_t){16, 288, 37, 436, 65535, 1, 1}, (game_action_t){16, 289, 26, 437, 65535, 1, 1}, (game_action_t){16, 290, 28, 438, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 291, 32, 440, 65535, 1, 1}, (game_action_t){16, 293, 25, 446, 441, 1, 5}, (game_action_t){16, 294, 32, 446, 442, 1, 5}, (game_action_t){16, 295, 39, 443, 444, 1, 5}, (game_action_t){16, 296, 31, 446, 65535, 1, 1}, (game_action_t){16, 297, 31, 446, 445, 1, 5}, (game_action_t){16, 298, 31, 446, 65535, 1, 5}, (game_action_t){16, 299, 21, 449, 447, 1, 3}, (game_action_t){16, 300, 24, 449, 448, 1, 3}, (game_action_t){16, 301, 28, 449, 65535, 1, 3}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 302, 32, 451, 65535, 1, 1}, (game_action_t){16, 304, 40, 452, 65535, 1, 1}, (game_action_t){2, 21, 0, 65535, 65535, 1, 1}, (game_action_t){1, 3, 0, 454, 65535, 1, 1}, (game_action_t){18, 305, 30, 65535, 65535, 1, 1}, (game_action_t){16, 306, 30, 456, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 307, 20, 65535, 458, 1, 4}, (game_action_t){18, 305, 30, 65535, 459, 1, 4}, (game_action_t){16, 308, 26, 65535, 460, 1, 4}, (game_action_t){16, 309, 27, 65535, 65535, 1, 4}, (game_action_t){16, 310, 32, 462, 65535, 1, 1}, (game_action_t){16, 312, 29, 463, 65535, 1, 1}, (game_action_t){16, 313, 40, 464, 65535, 1, 1}, (game_action_t){16, 314, 33, 465, 65535, 1, 1}, (game_action_t){16, 315, 35, 466, 65535, 1, 1}, (game_action_t){16, 316, 38, 467, 65535, 1, 1}, (game_action_t){6, 0, 0, 468, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 317, 40, 470, 65535, 1, 1}, (game_action_t){16, 318, 29, 471, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 319, 35, 473, 65535, 1, 1}, (game_action_t){16, 321, 34, 475, 474, 1, 2}, (game_action_t){16, 322, 34, 475, 65535, 1, 2}, (game_action_t){16, 14, 17, 476, 65535, 1, 1}, (game_action_t){16, 323, 38, 477, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 324, 27, 479, 65535, 1, 1}, (game_action_t){16, 14, 17, 480, 65535, 1, 1}, (game_action_t){16, 326, 30, 481, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 327, 33, 483, 65535, 1, 1}, (game_action_t){16, 14, 17, 484, 65535, 1, 1}, (game_action_t){2, 22, 0, 65535, 65535, 1, 1}, (game_action_t){16, 329, 27, 486, 65535, 1, 1}, (game_action_t){16, 330, 24, 487, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){18, 331, 26, 489, 65535, 1, 1}, (game_action_t){16, 332, 36, 490, 65535, 1, 1}, (game_action_t){2, 23, 0, 65535, 65535, 1, 1}, (game_action_t){16, 333, 35, 492, 65535, 1, 1}, (game_action_t){16, 335, 40, 493, 65535, 1, 1}, (game_action_t){2, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 336, 30, 495, 65535, 1, 1}, (game_action_t){16, 338, 36, 497, 496, 1, 2}, (game_action_t){16, 339, 33, 497, 65535, 1, 2}, (game_action_t){16, 340, 29, 499, 498, 1, 2}, (game_action_t){16, 341, 31, 499, 65535, 1, 2}, (game_action_t){16, 342, 30, 500, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){16, 344, 31, 503, 65535, 1, 1}, (game_action_t){16, 346, 38, 504, 65535, 1, 1}, (game_action_t){16, 347, 40, 505, 65535, 1, 1}, (game_action_t){16, 348, 30, 506, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 349, 39, 508, 65535, 1, 1}, (game_action_t){16, 351, 37, 509, 65535, 1, 1}, (game_action_t){16, 352, 38, 510, 65535, 1, 1}, (game_action_t){16, 353, 39, 511, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 354, 40, 513, 65535, 1, 1}, (game_action_t){16, 356, 40, 514, 65535, 1, 1}, (game_action_t){16, 357, 39, 515, 65535, 1, 1}, (game_action_t){16, 358, 38, 516, 65535, 1, 1}, (game_action_t){16, 359, 35, 517, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 360, 35, 519, 65535, 1, 1}, (game_action_t){16, 362, 40, 520, 65535, 1, 1}, (game_action_t){16, 363, 33, 521, 65535, 1, 1}, (game_action_t){16, 364, 40, 522, 65535, 1, 1}, (game_action_t){16, 365, 39, 523, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){2, 40, 0, 65535, 65535, 1, 1}, (game_action_t){2, 44, 0, 65535, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 527, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 368, 22, 533, 530, 1, 4}, (game_action_t){16, 370, 28, 533, 531, 1, 4}, (game_action_t){16, 371, 32, 533, 532, 1, 4}, (game_action_t){16, 372, 23, 533, 65535, 1, 4}, (game_action_t){16, 373, 27, 535, 534, 1, 2}, (game_action_t){16, 374, 27, 535, 65535, 1, 2}, (game_action_t){19, 375, 34, 65535, 65535, 1, 1}, (game_action_t){16, 376, 24, 538, 537, 1, 2}, (game_action_t){16, 378, 19, 538, 65535, 1, 2}, (game_action_t){16, 379, 38, 539, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 543, 4, 8}, (game_action_t){16, 380, 160, 542, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 382, 21, 65535, 544, 1, 8}, (game_action_t){16, 383, 40, 545, 546, 1, 8}, (game_action_t){16, 384, 20, 65535, 65535, 1, 1}, (game_action_t){16, 385, 29, 65535, 547, 1, 8}, (game_action_t){16, 386, 37, 548, 65535, 1, 8}, (game_action_t){16, 387, 35, 549, 65535, 1, 1}, (game_action_t){16, 388, 24, 65535, 65535, 1, 1}, (game_action_t){16, 389, 27, 552, 551, 1, 2}, (game_action_t){16, 391, 35, 552, 65535, 1, 2}, (game_action_t){16, 392, 36, 553, 65535, 1, 1}, (game_action_t){16, 393, 33, 554, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 394, 160, 556, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){2, 45, 0, 65535, 65535, 1, 1}, (game_action_t){2, 28, 0, 65535, 65535, 1, 1}, (game_action_t){16, 368, 22, 563, 560, 1, 4}, (game_action_t){16, 370, 28, 563, 561, 1, 4}, (game_action_t){16, 371, 32, 563, 562, 1, 4}, (game_action_t){16, 372, 23, 563, 65535, 1, 4}, (game_action_t){16, 373, 27, 565, 564, 1, 2}, (game_action_t){16, 374, 27, 565, 65535, 1, 2}, (game_action_t){19, 375, 34, 65535, 65535, 1, 1}, (game_action_t){16, 376, 24, 568, 567, 1, 2}, (game_action_t){16, 378, 19, 568, 65535, 1, 2}, (game_action_t){16, 379, 38, 569, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 571, 4, 7}, (game_action_t){16, 396, 32, 572, 573, 1, 7}, (game_action_t){16, 397, 25, 65535, 65535, 1, 1}, (game_action_t){16, 398, 40, 65535, 574, 1, 7}, (game_action_t){16, 399, 35, 65535, 65535, 1, 7}, (game_action_t){16, 389, 27, 577, 576, 1, 2}, (game_action_t){16, 391, 35, 577, 65535, 1, 2}, (game_action_t){16, 392, 36, 578, 65535, 1, 1}, (game_action_t){16, 393, 33, 579, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 400, 23, 581, 65535, 1, 1}, (game_action_t){16, 402, 30, 582, 65535, 1, 1}, (game_action_t){16, 403, 31, 583, 65535, 1, 1}, (game_action_t){2, 39, 0, 65535, 65535, 1, 1}, (game_action_t){2, 29, 0, 65535, 65535, 1, 1}, (game_action_t){2, 30, 0, 65535, 65535, 1, 1}, (game_action_t){16, 368, 22, 590, 587, 1, 4}, (game_action_t){16, 370, 28, 590, 588, 1, 4}, (game_action_t){16, 371, 32, 590, 589, 1, 4}, (game_action_t){16, 372, 23, 590, 65535, 1, 4}, (game_action_t){16, 373, 27, 592, 591, 1, 2}, (game_action_t){16, 374, 27, 592, 65535, 1, 2}, (game_action_t){19, 375, 34, 65535, 65535, 1, 1}, (game_action_t){16, 376, 24, 595, 594, 1, 2}, (game_action_t){16, 378, 19, 595, 65535, 1, 2}, (game_action_t){16, 379, 38, 596, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 598, 4, 7}, (game_action_t){16, 396, 32, 599, 600, 1, 7}, (game_action_t){16, 397, 25, 65535, 65535, 1, 1}, (game_action_t){16, 398, 40, 65535, 601, 1, 7}, (game_action_t){16, 399, 35, 65535, 65535, 1, 7}, (game_action_t){16, 389, 27, 604, 603, 1, 2}, (game_action_t){16, 391, 35, 604, 65535, 1, 2}, (game_action_t){16, 392, 36, 605, 65535, 1, 1}, (game_action_t){16, 393, 33, 606, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 405, 28, 608, 65535, 1, 1}, (game_action_t){16, 406, 37, 609, 65535, 1, 1}, (game_action_t){16, 407, 40, 610, 65535, 1, 1}, (game_action_t){16, 408, 40, 611, 65535, 1, 1}, (game_action_t){16, 409, 34, 612, 65535, 1, 1}, (game_action_t){16, 410, 37, 613, 65535, 1, 1}, (game_action_t){16, 411, 38, 614, 65535, 1, 1}, (game_action_t){6, 0, 0, 615, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 31, 0, 65535, 65535, 1, 1}, (game_action_t){2, 32, 0, 65535, 65535, 1, 1}, (game_action_t){16, 368, 22, 622, 619, 1, 4}, (game_action_t){16, 370, 28, 622, 620, 1, 4}, (game_action_t){16, 371, 32, 622, 621, 1, 4}, (game_action_t){16, 372, 23, 622, 65535, 1, 4}, (game_action_t){16, 373, 27, 624, 623, 1, 2}, (game_action_t){16, 374, 27, 624, 65535, 1, 2}, (game_action_t){19, 375, 34, 65535, 65535, 1, 1}, (game_action_t){16, 376, 24, 627, 626, 1, 2}, (game_action_t){16, 378, 19, 627, 65535, 1, 2}, (game_action_t){16, 379, 38, 628, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 630, 4, 7}, (game_action_t){16, 396, 32, 631, 632, 1, 7}, (game_action_t){16, 397, 25, 65535, 65535, 1, 1}, (game_action_t){16, 398, 40, 65535, 633, 1, 7}, (game_action_t){16, 399, 35, 65535, 65535, 1, 7}, (game_action_t){16, 389, 27, 636, 635, 1, 2}, (game_action_t){16, 391, 35, 636, 65535, 1, 2}, (game_action_t){16, 392, 36, 637, 65535, 1, 1}, (game_action_t){16, 393, 33, 638, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 400, 23, 640, 65535, 1, 1}, (game_action_t){16, 402, 30, 641, 65535, 1, 1}, (game_action_t){16, 403, 31, 642, 65535, 1, 1}, (game_action_t){2, 39, 0, 65535, 65535, 1, 1}, (game_action_t){16, 413, 32, 650, 644, 1, 6}, (game_action_t){16, 414, 34, 645, 646, 1, 6}, (game_action_t){16, 415, 25, 650, 65535, 1, 1}, (game_action_t){16, 416, 38, 650, 647, 1, 6}, (game_action_t){17, 417, 35, 650, 648, 1, 6}, (game_action_t){16, 418, 35, 650, 649, 1, 6}, (game_action_t){17, 419, 31, 650, 65535, 1, 6}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 33, 0, 65535, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 368, 22, 657, 654, 1, 4}, (game_action_t){16, 370, 28, 657, 655, 1, 4}, (game_action_t){16, 371, 32, 657, 656, 1, 4}, (game_action_t){16, 372, 23, 657, 65535, 1, 4}, (game_action_t){16, 373, 27, 659, 658, 1, 2}, (game_action_t){16, 374, 27, 659, 65535, 1, 2}, (game_action_t){19, 375, 34, 65535, 65535, 1, 1}, (game_action_t){16, 376, 24, 662, 661, 1, 2}, (game_action_t){16, 378, 19, 662, 65535, 1, 2}, (game_action_t){16, 379, 38, 663, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 665, 4, 7}, (game_action_t){16, 396, 32, 666, 667, 1, 7}, (game_action_t){16, 397, 25, 65535, 65535, 1, 1}, (game_action_t){16, 398, 40, 65535, 668, 1, 7}, (game_action_t){16, 399, 35, 65535, 65535, 1, 7}, (game_action_t){16, 389, 27, 671, 670, 1, 2}, (game_action_t){16, 391, 35, 671, 65535, 1, 2}, (game_action_t){16, 392, 36, 672, 65535, 1, 1}, (game_action_t){16, 393, 33, 673, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 421, 38, 675, 65535, 1, 1}, (game_action_t){16, 422, 36, 676, 65535, 1, 1}, (game_action_t){16, 423, 28, 677, 65535, 1, 1}, (game_action_t){6, 0, 0, 678, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 400, 96, 680, 65535, 1, 1}, (game_action_t){16, 424, 37, 681, 65535, 1, 1}, (game_action_t){16, 425, 39, 682, 65535, 1, 1}, (game_action_t){16, 426, 25, 65535, 65535, 1, 1}, (game_action_t){2, 34, 0, 65535, 65535, 1, 1}, (game_action_t){2, 35, 0, 65535, 65535, 1, 1}, (game_action_t){17, 429, 38, 65535, 686, 1, 4}, (game_action_t){16, 431, 28, 65535, 687, 1, 4}, (game_action_t){16, 432, 29, 65535, 688, 1, 4}, (game_action_t){16, 433, 37, 65535, 65535, 1, 4}, (game_action_t){16, 434, 39, 690, 65535, 1, 1}, (game_action_t){16, 435, 25, 691, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 436, 40, 65535, 65535, 1, 1}, (game_action_t){2, 36, 0, 65535, 65535, 1, 1}, (game_action_t){2, 37, 0, 65535, 65535, 1, 1}, (game_action_t){2, 38, 0, 65535, 65535, 1, 1}, (game_action_t){16, 440, 39, 697, 65535, 1, 1}, (game_action_t){16, 441, 31, 698, 65535, 1, 1}, (game_action_t){16, 121, 20, 699, 65535, 1, 1}, (game_action_t){16, 442, 29, 700, 65535, 1, 1}, (game_action_t){16, 443, 28, 701, 65535, 1, 1}, (game_action_t){16, 444, 36, 702, 65535, 1, 1}, (game_action_t){16, 445, 36, 703, 65535, 1, 1}, (game_action_t){16, 446, 22, 704, 65535, 1, 1}, (game_action_t){16, 14, 17, 705, 65535, 1, 1}, (game_action_t){16, 447, 33, 706, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 448, 30, 708, 65535, 1, 1}, (game_action_t){16, 449, 33, 709, 65535, 1, 1}, (game_action_t){16, 450, 38, 710, 65535, 1, 1}, (game_action_t){16, 451, 25, 711, 65535, 1, 1}, (game_action_t){16, 14, 17, 712, 65535, 1, 1}, (game_action_t){16, 444, 36, 713, 65535, 1, 1}, (game_action_t){16, 452, 26, 714, 65535, 1, 1}, (game_action_t){16, 453, 39, 715, 65535, 1, 1}, (game_action_t){16, 454, 25, 716, 65535, 1, 1}, (game_action_t){16, 455, 40, 717, 65535, 1, 1}, (game_action_t){16, 456, 34, 718, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 434, 39, 720, 65535, 1, 1}, (game_action_t){16, 435, 25, 721, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 457, 25, 723, 65535, 1, 1}, (game_action_t){16, 458, 38, 724, 65535, 1, 1}, (game_action_t){16, 14, 17, 725, 65535, 1, 1}, (game_action_t){16, 459, 22, 726, 65535, 1, 1}, (game_action_t){16, 460, 40, 727, 65535, 1, 1}, (game_action_t){16, 461, 38, 65535, 65535, 1, 1}, (game_action_t){16, 462, 36, 729, 65535, 1, 1}, (game_action_t){16, 464, 38, 730, 65535, 1, 1}, (game_action_t){16, 465, 38, 731, 65535, 1, 1}, (game_action_t){16, 466, 25, 732, 65535, 1, 1}, (game_action_t){16, 467, 39, 733, 65535, 1, 1}, (game_action_t){16, 468, 39, 734, 65535, 1, 1}, (game_action_t){16, 469, 38, 735, 65535, 1, 1}, (game_action_t){16, 470, 40, 736, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 471, 29, 738, 65535, 1, 1}, (game_action_t){16, 473, 37, 739, 65535, 1, 1}, (game_action_t){16, 467, 39, 740, 65535, 1, 1}, (game_action_t){16, 468, 39, 741, 65535, 1, 1}, (game_action_t){16, 469, 38, 742, 65535, 1, 1}, (game_action_t){16, 470, 40, 743, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 474, 27, 745, 65535, 1, 1}, (game_action_t){16, 475, 39, 746, 65535, 1, 1}, (game_action_t){16, 476, 28, 747, 65535, 1, 1}, (game_action_t){16, 477, 37, 748, 65535, 1, 1}, (game_action_t){16, 478, 38, 749, 65535, 1, 1}, (game_action_t){16, 479, 40, 750, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){100, 3, 0, 752, 65535, 1, 1}, (game_action_t){16, 480, 31, 65535, 65535, 1, 1}, (game_action_t){16, 481, 28, 754, 65535, 1, 1}, (game_action_t){16, 483, 40, 755, 65535, 1, 1}, (game_action_t){16, 484, 33, 756, 65535, 1, 1}, (game_action_t){16, 485, 38, 757, 65535, 1, 1}, (game_action_t){16, 486, 37, 758, 65535, 1, 1}, (game_action_t){16, 487, 27, 759, 65535, 1, 1}, (game_action_t){16, 488, 39, 760, 65535, 1, 1}, (game_action_t){16, 489, 31, 761, 65535, 1, 1}, (game_action_t){6, 0, 0, 762, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 490, 33, 764, 65535, 1, 1}, (game_action_t){16, 491, 32, 765, 65535, 1, 1}, (game_action_t){16, 492, 38, 766, 65535, 1, 1}, (game_action_t){16, 493, 30, 767, 65535, 1, 1}, (game_action_t){6, 0, 0, 768, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 494, 30, 770, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 495, 26, 773, 772, 1, 2}, (game_action_t){16, 496, 29, 773, 65535, 1, 2}, (game_action_t){100, 2, 0, 774, 65535, 1, 1}, (game_action_t){19, 497, 35, 65535, 775, 1, 3}, (game_action_t){19, 498, 28, 65535, 776, 1, 3}, (game_action_t){19, 499, 30, 65535, 65535, 1, 3}, (game_action_t){16, 500, 21, 780, 778, 1, 3}, (game_action_t){16, 502, 24, 780, 779, 1, 3}, (game_action_t){16, 503, 31, 780, 65535, 1, 3}, (game_action_t){16, 14, 96, 65535, 65535, 1, 1}, (game_action_t){16, 504, 25, 782, 65535, 1, 1}, (game_action_t){16, 505, 33, 783, 65535, 1, 1}, (game_action_t){16, 506, 30, 784, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 507, 21, 789, 786, 1, 3}, (game_action_t){16, 508, 40, 789, 787, 1, 3}, (game_action_t){16, 509, 40, 788, 65535, 1, 3}, (game_action_t){16, 510, 27, 789, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 511, 31, 791, 65535, 1, 1}, (game_action_t){16, 512, 34, 792, 65535, 1, 1}, (game_action_t){16, 513, 36, 793, 65535, 1, 1}, (game_action_t){16, 514, 23, 794, 65535, 1, 1}, (game_action_t){16, 515, 29, 795, 65535, 1, 1}, (game_action_t){16, 516, 37, 796, 65535, 1, 1}, (game_action_t){16, 14, 17, 797, 65535, 1, 1}, (game_action_t){16, 517, 38, 798, 65535, 1, 1}, (game_action_t){6, 0, 0, 799, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 518, 32, 801, 65535, 1, 1}, (game_action_t){16, 519, 35, 802, 65535, 1, 1}, (game_action_t){16, 520, 26, 803, 65535, 1, 1}, (game_action_t){16, 521, 40, 804, 65535, 1, 1}, (game_action_t){16, 522, 38, 805, 65535, 1, 1}, (game_action_t){16, 523, 28, 806, 65535, 1, 1}, (game_action_t){6, 0, 0, 807, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}};

game_state_t all_states[] = {(game_state_t){.entry_series_id=0, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=6}}, .input_series={(game_user_in_t){.text_addr=15, .result_action_id=15},(game_user_in_t){.text_addr=16, .result_action_id=16},(game_user_in_t){.text_addr=17, .result_action_id=17},(game_user_in_t){.text_addr=19, .result_action_id=21},(game_user_in_t){.text_addr=22, .result_action_id=25},(game_user_in_t){.text_addr=23, .result_action_id=26}}, .other_series={}}, (game_state_t){.entry_series_id=27, .timer_series_len=0, .input_series_len=0, .other_series_len=2, .timer_series={}, .input_series={}, .other_series={(game_other_in_t){.type_id=2, .result_action_id=29},(game_other_in_t){.type_id=3, .result_action_id=33}}}, (game_state_t){.entry_series_id=40, .timer_series_len=1, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=42}}, .input_series={(game_user_in_t){.text_addr=36, .result_action_id=43},(game_user_in_t){.text_addr=52, .result_action_id=50},(game_user_in_t){.text_addr=55, .result_action_id=61},(game_user_in_t){.text_addr=64, .result_action_id=67}}, .other_series={}}, (game_state_t){.entry_series_id=70, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=75},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=80}}, .input_series={(game_user_in_t){.text_addr=19, .result_action_id=81},(game_user_in_t){.text_addr=70, .result_action_id=85},(game_user_in_t){.text_addr=71, .result_action_id=86}}, .other_series={}}, (game_state_t){.entry_series_id=87, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=92},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=97}}, .input_series={(game_user_in_t){.text_addr=78, .result_action_id=98}}, .other_series={}}, (game_state_t){.entry_series_id=103, .timer_series_len=2, .input_series_len=2, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=109},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=115}}, .input_series={(game_user_in_t){.text_addr=86, .result_action_id=116},(game_user_in_t){.text_addr=17, .result_action_id=121}}, .other_series={}}, (game_state_t){.entry_series_id=125, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=129},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=133}}, .input_series={(game_user_in_t){.text_addr=91, .result_action_id=134},(game_user_in_t){.text_addr=92, .result_action_id=139},(game_user_in_t){.text_addr=94, .result_action_id=144}}, .other_series={}}, (game_state_t){.entry_series_id=149, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=150}}, .input_series={(game_user_in_t){.text_addr=97, .result_action_id=151},(game_user_in_t){.text_addr=98, .result_action_id=153},(game_user_in_t){.text_addr=99, .result_action_id=155},(game_user_in_t){.text_addr=100, .result_action_id=157},(game_user_in_t){.text_addr=101, .result_action_id=159},(game_user_in_t){.text_addr=102, .result_action_id=160}}, .other_series={}}, (game_state_t){.entry_series_id=161, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=162}}, .input_series={(game_user_in_t){.text_addr=105, .result_action_id=163},(game_user_in_t){.text_addr=107, .result_action_id=168},(game_user_in_t){.text_addr=109, .result_action_id=173},(game_user_in_t){.text_addr=111, .result_action_id=178},(game_user_in_t){.text_addr=19, .result_action_id=179}}, .other_series={}}, (game_state_t){.entry_series_id=183, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=184}}, .input_series={(game_user_in_t){.text_addr=107, .result_action_id=185},(game_user_in_t){.text_addr=112, .result_action_id=190},(game_user_in_t){.text_addr=114, .result_action_id=191},(game_user_in_t){.text_addr=116, .result_action_id=196},(game_user_in_t){.text_addr=118, .result_action_id=201},(game_user_in_t){.text_addr=19, .result_action_id=203}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=207}}, .input_series={(game_user_in_t){.text_addr=107, .result_action_id=208},(game_user_in_t){.text_addr=112, .result_action_id=213},(game_user_in_t){.text_addr=114, .result_action_id=214},(game_user_in_t){.text_addr=116, .result_action_id=219},(game_user_in_t){.text_addr=118, .result_action_id=224},(game_user_in_t){.text_addr=19, .result_action_id=225}}, .other_series={}}, (game_state_t){.entry_series_id=229, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=247},(game_timer_t){.duration=1152, .recurring=0, .result_action_id=252}}, .input_series={(game_user_in_t){.text_addr=144, .result_action_id=260},(game_user_in_t){.text_addr=150, .result_action_id=266},(game_user_in_t){.text_addr=157, .result_action_id=273},(game_user_in_t){.text_addr=163, .result_action_id=279}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=1152, .recurring=0, .result_action_id=287},(game_timer_t){.duration=256, .recurring=0, .result_action_id=295}}, .input_series={(game_user_in_t){.text_addr=178, .result_action_id=302},(game_user_in_t){.text_addr=184, .result_action_id=308},(game_user_in_t){.text_addr=195, .result_action_id=319}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=480, .recurring=0, .result_action_id=329},(game_timer_t){.duration=38400, .recurring=0, .result_action_id=335}}, .input_series={(game_user_in_t){.text_addr=213, .result_action_id=340},(game_user_in_t){.text_addr=223, .result_action_id=350},(game_user_in_t){.text_addr=231, .result_action_id=359},(game_user_in_t){.text_addr=235, .result_action_id=365}}, .other_series={}}, (game_state_t){.entry_series_id=369, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=244, .result_action_id=375}}, .other_series={}}, (game_state_t){.entry_series_id=386, .timer_series_len=0, .input_series_len=2, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=256, .result_action_id=391},(game_user_in_t){.text_addr=260, .result_action_id=396}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=260, .result_action_id=400}}, .other_series={}}, (game_state_t){.entry_series_id=404, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=268, .result_action_id=409},(game_user_in_t){.text_addr=273, .result_action_id=413},(game_user_in_t){.text_addr=279, .result_action_id=418}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=422},(game_timer_t){.duration=90240, .recurring=0, .result_action_id=426}}, .input_series={(game_user_in_t){.text_addr=231, .result_action_id=428},(game_user_in_t){.text_addr=287, .result_action_id=434},(game_user_in_t){.text_addr=292, .result_action_id=439},(game_user_in_t){.text_addr=303, .result_action_id=450}}, .other_series={}}, (game_state_t){.entry_series_id=453, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=960, .recurring=0, .result_action_id=455},(game_timer_t){.duration=64, .recurring=0, .result_action_id=457}}, .input_series={(game_user_in_t){.text_addr=311, .result_action_id=461}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=469}}, .input_series={(game_user_in_t){.text_addr=320, .result_action_id=472},(game_user_in_t){.text_addr=325, .result_action_id=478},(game_user_in_t){.text_addr=328, .result_action_id=482}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=0, .other_series_len=2, .timer_series={}, .input_series={}, .other_series={(game_other_in_t){.type_id=0, .result_action_id=485},(game_other_in_t){.type_id=1, .result_action_id=488}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=334, .result_action_id=491}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=337, .result_action_id=494},(game_user_in_t){.text_addr=343, .result_action_id=501},(game_user_in_t){.text_addr=345, .result_action_id=502}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=350, .result_action_id=507},(game_user_in_t){.text_addr=355, .result_action_id=512},(game_user_in_t){.text_addr=361, .result_action_id=518}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=9600, .recurring=0, .result_action_id=526},(game_timer_t){.duration=21120, .recurring=0, .result_action_id=528}}, .input_series={(game_user_in_t){.text_addr=366, .result_action_id=524},(game_user_in_t){.text_addr=367, .result_action_id=525},(game_user_in_t){.text_addr=369, .result_action_id=529},(game_user_in_t){.text_addr=377, .result_action_id=536},(game_user_in_t){.text_addr=381, .result_action_id=540},(game_user_in_t){.text_addr=390, .result_action_id=550}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=558}}, .input_series={(game_user_in_t){.text_addr=395, .result_action_id=557},(game_user_in_t){.text_addr=369, .result_action_id=559},(game_user_in_t){.text_addr=377, .result_action_id=566},(game_user_in_t){.text_addr=381, .result_action_id=570},(game_user_in_t){.text_addr=390, .result_action_id=575},(game_user_in_t){.text_addr=401, .result_action_id=580}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=585}}, .input_series={(game_user_in_t){.text_addr=404, .result_action_id=584},(game_user_in_t){.text_addr=369, .result_action_id=586},(game_user_in_t){.text_addr=377, .result_action_id=593},(game_user_in_t){.text_addr=381, .result_action_id=597},(game_user_in_t){.text_addr=390, .result_action_id=602}}, .other_series={}}, (game_state_t){.entry_series_id=607, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=617}}, .input_series={(game_user_in_t){.text_addr=412, .result_action_id=616},(game_user_in_t){.text_addr=369, .result_action_id=618},(game_user_in_t){.text_addr=377, .result_action_id=625},(game_user_in_t){.text_addr=381, .result_action_id=629},(game_user_in_t){.text_addr=390, .result_action_id=634},(game_user_in_t){.text_addr=401, .result_action_id=639}}, .other_series={}}, (game_state_t){.entry_series_id=643, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=652}}, .input_series={(game_user_in_t){.text_addr=420, .result_action_id=651},(game_user_in_t){.text_addr=369, .result_action_id=653},(game_user_in_t){.text_addr=377, .result_action_id=660},(game_user_in_t){.text_addr=381, .result_action_id=664},(game_user_in_t){.text_addr=390, .result_action_id=669}}, .other_series={}}, (game_state_t){.entry_series_id=679, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=674}}, .input_series={(game_user_in_t){.text_addr=427, .result_action_id=683},(game_user_in_t){.text_addr=428, .result_action_id=684},(game_user_in_t){.text_addr=430, .result_action_id=685}}, .other_series={}}, (game_state_t){.entry_series_id=692, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=689}}, .input_series={(game_user_in_t){.text_addr=437, .result_action_id=693},(game_user_in_t){.text_addr=438, .result_action_id=694},(game_user_in_t){.text_addr=439, .result_action_id=695}}, .other_series={}}, (game_state_t){.entry_series_id=696, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=707, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=722, .timer_series_len=1, .input_series_len=2, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=719}}, .input_series={(game_user_in_t){.text_addr=463, .result_action_id=728},(game_user_in_t){.text_addr=472, .result_action_id=737}}, .other_series={}}, (game_state_t){.entry_series_id=744, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=751, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=482, .result_action_id=753}}, .other_series={}}, (game_state_t){.entry_series_id=763, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=771, .timer_series_len=1, .input_series_len=0, .other_series_len=0, .timer_series={(game_timer_t){.duration=320, .recurring=0, .result_action_id=769}}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=781}}, .input_series={(game_user_in_t){.text_addr=501, .result_action_id=777}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=501, .result_action_id=785}}, .other_series={}}, (game_state_t){.entry_series_id=790, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=800, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=541, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=555, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}};

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
#define STATE_ID_JUSTJOINED 36
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
#define OTHER_ACTION_CUSTOMSTATEUSERNAME 0
#define OTHER_ACTION_NAMESEARCH 1
#define OTHER_ACTION_SET_CONNECTABLE 2
#define OTHER_ACTION_TURN_ON_THE_LIGHTS_TO_REPRESENT_FILE_STATE 3
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
