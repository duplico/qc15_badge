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

uint16_t all_actions_len = 831;
uint16_t main_text_len = 445;
#define MAIN_TEXT_LEN 445
#define AUX_TEXT_LEN 126
uint16_t all_states_len = 48;
#define MAX_INPUTS 6
#define MAX_TIMERS 2
#define MAX_OTHERS 5
uint16_t all_anims_len = 4;

// lightsSolidWhite, lightsWhiteFader, animSpinBlue, whiteDiscovery
uint8_t main_text[][25] = {"y;EJpnzb{RaKZT)|c9[.<;V5","SE{;?.&j]/x'8k```sS{HOc!","","Hello?","Are you broken?","Reboot.","INITIALIZING","Initialize.","....................","Completed.","I'm confused","Set state.","Scan what, %s?","Nah, nothing.","But something was there.","I'm so confused.","WOAH. WOAH.","So I'm not alone.","(No offense)","You will never believe","what I learned.","Welcome to customer","support, how may I help?","I'm sorry to hear that!","Badge doesn't work.","I'll be glad to help.","First do a reset.","Please unplug your","badge, wait three","seconds, then plug it","back in.","I'm sorry I have failed","to provide you adequate","assistance at this time.","Our supervisor is","available on 3rd","Tuesdays between the","hours of 3am and 3:17am","in whatever timezone you","are in.","Supervisor","Goodbye.","Have you?","I tried a reboot.","Have you really?","You think you know more","about this than me?","Do it again.","I'll wait.","I'm sorry, I cannot","discuss personal matters","with you.","You speak English!","...","HellofuGLZjLM","HeEVczyYTllo","pPHhSUyEihQyHORpxbkC","vsOuFKtKJyXcRskMujiZ","Speak English!","Learn","broke455gfr","you broken6!","Are you...","irZfuqKJSHxkSYIxmIKL","AKcvEXVlddnmWyrfraSi","BUFFER OVERFLOW","I give up.","Rebooting","Speak helloFxgGj","EngliHeNnllo","Speakeng","HelloddnmWyr","yxyaBKhyUSOCJEitQrwK","XXwLJfZlPTWBqmOTuEWY","Getting closer...","Words learn","Heword learnHmoeQuq","LVPSIndOsZTeDgEttZux","Earn wordslo","Keep learning","Load English","ERR: Not in state mode","Store wordset","Set state:","ERR: State not found.","Language","Mode","First Run","AR112-1","ISO-3166-1.2","BCP-47","Value:","ERR: \"You pick.\"","You pick.","ERR: \"English\"","English","ERR: \"Game mode\"","Game mode","Value stored.","US","Help","You wish.","God mode","Datei nicht gefunden.","zh-DE","Neustart...","en-boont-latn-021","ERR: \"English\" invalid.","Hello! I am -","....","Waitaminute.","What's going on?","Sysclock just says","12:00","Ohh, crap.","I've lost main memory.","CRAP!","This is not good.","And then who the hell","are you?","Still there?","Screw it.","We're done.","My name is...","%s?","Good name for a monkey.","Well %s I have no","idea what's going on.","Seriously?","Who are you?","I asked you first.","For crying out loud...","What are you?","I'm not a *thing*. Well","ok, I mean... shut up.","You going to tell me","your name or not?","Everything!","What do you do?","I think.","No matter now.","That's it.","Nevermind.","Waste my time...","You're killing me.","See you, human.","Sigh...","It's like I just woke up","What do you mean?","I was doing something.","Holding something.","Something important.","But now I don't know.","Ah, this I know.","What's your name?","Some call me awesome","But you can call me...","%s","Now we know each other.","Sort of.","Really wish I knew more","than that.","WHAT. DID YOU. SAY?","You're a badge.","I'm brilliant.","And I need help.","From a monkey.","This SUCKS.","I feel stuck.","You there, %s?","Ok, later then.","My status?","Status","Damned confused.","That's my status.","WHAT?","You're my badge!","You don't OWN me!","If anything, I'd own YOU","All superior beings,","raise your processing","power to 6.77 exawatts.","Yeah, thought so.","Do YOU have lights?","Do you have lights?","See how that sounds?","That's how you sound.","Oh wait.","What do you need?","Guess I'm glad you","asked.","WOHOO!","I did that.","Me. Booyah!","So awesome.","Ugh %s that's so","like you.","That's really bright.","I do a thing, and you","complain.","Fine.","Better?","Ok yes that's better.","I'm lost.","I remember holding","something.......","And I wasn't alone.","Is that so hard to","believe?","Like friends?","Hey, I'm pretty awesome.","But now I have no one.","Oh, monkey...","Hey, I'm here.","You don't count.","I mean more like me.","I've lost control.","And lost my goal.","Now I need you. UGH!","I protected something.","What was your goal?","Part of it, anyway.","I assume I still have","it, whatever it is.","HEY!","What's in it for me?","Gotta be a \"shock the","idiot\" button","somewhere...","Try pushing buttons.","Thank you!","Ok, I'll help.","One problem...","I don't know what to do.","Humm...","I'm not sure.","What's your goal?","I'm sensing something","out there.","And in here.","More? Like what?","Can you do more?","Brew tea?","Dolt.","Scan? Scan what?","Scan","Hey %s!","Eh, nevermind.","Hey!","I just realized!","What is it?","There's no 2!","In my code, there are 1s","and 0s but no 2s.","Can you count to 2?","Don't strain yourself.","You, me, or \"out there\"?","Ok, I scanned your face.","It was funny.","Uh, ok. Scanning...","You.","Lots of awesome...","...and something else.","Really? Ok.","Me.","Nope. Nothing.","Right. Looking...","Out there.","...nothing.","Oh well.","%s.","Someone's out there.","Connecting to what?","Try connecting","There are some markings.","Actually, yes.","Anything about me?","You're funny-lookin.","But are nice?","I'm skeptical.","Tell me about...","I sure hope so.","Got it. Let's do it.","If not, ask other meat","bags with more patience.","Let's do this!","Another being, like me.","...that badge?","I can see names, now.","And we have something.","A file. Or part of one.","It seems there's a file.","...your goal?","Broken up. I've a piece.","Others have pieces too.","But they're encrypted.","We HAVE to open it.","Well I'm not alone.","...you?","I protect this file, but","it is fragmented.","And locked. Key is gone.","Time to brute force it!","Why's the file locked?","Do you have speakers?","Dunno.","Anyone around?","Scanning...","I see %d badges.","Giggity.","Let's connect.","Connecting to network.","Status check","Busy!","More bling?","More with lights?","I'll try some things","You give a shout.","COLORFINDER","How old are you?","Let's check.","Why, you lonely?","Hrmm...","Checking...","Ok.","Studying quantum","knitting.","Not gay enough yet?","It's...","How's the file?","You know what?","Let's try this.","Do you have it all?","Practicing my showtunes.","What's YOUR status?","No, I don't.","By my calculations, I","have 1/16th of the file.","I see other bits when we","connect to others.","At some point, we all","need to come together.","Hey $badgname","Don't be afraid.","What IS the file?","No matter, the file is","something special to","some people.","Are you familiar with","the shadow org known as","QUEERCON?","Yes","No","%s the awesome.","Who's asking?","Anyway, it's special to","QUEERCON.","How well do you know em?","Just joined.","From the beginning.","It's been a while.","Well, they're like this","exclusive club.","Sorry, I mean","\"inclusive\".","Anyway, this file is","like their long lost","sigil.","I dunno any more.","Well, welcome.","Glad you're here.","Meat bags like you are","so handy.","from them.","It's like a mascot from","long ago.","Even I've never seen it.","So let's crack it!","Really???","Never seen you before.","......","Were you only let out to","clean up or something?","Don't call me badge!","Watch it, badge!","Really need to install","that \"shock the idiot\"","button...","Anyway, the file is the","original sigil, I hear.","So you should know it.","Mr. \"Been here forever\".","The smartest!","Smartass.","Finally a compliment.","Impressive!","I think 3 days with you","is my limit.","In the beginning, the","founders set a mascot.","I think that's the file.","Does that help?","So no, then.","What am I seeing?","The lights show how much","we have unlocked.","The more we unlock the","file, the more stable","the lights.","Also other stuff, go to","the badge talk.","There's a note...","\"To make closing","ceremony interesting.\"","Mean anything?","I'm in. And I found...","Something new.","I'm in. But I didn't","learn anything new.","No joy. Couldn't get in.","Nobody's here.","Connected!","%d badges around.","It'll need to be online.","Connect to a badge","Logging off...","Nope.","Wake up!","Ok, fine.","Now you be nicer!","Where were we?","Wha-?","...oh. My. GOD.","You want speakers?","What are we playing?","Bieber?","Fraggle Rock?","A capella hair metal?","I only play showtunes.","First off, rude.","Second, still rude.","I dunno...","About 15K of your years.","But younger than those","who made me."};

uint8_t aux_text[][25] = {"T3d1/0CH-LXn*~/;6p,v._;s","qVL5L5S#NETd.(_5?Bc(:Hb,","a+Xxf(4]9|PX+&t11R-InRNK",".B 4>09;2lq&bk{UTz+fgT#d","0z4i qbP!:>7 KU6FST`95[e",".!e.w>N|QdX.zK/a<LjU'+yE","Cucumber","^!LygYD&W83#XN&(e@\"(7u_r","5ESnMiz!O%MiCCKJ(/o%I^sk","cE+p#8xhG,iCl;2a|2y*:<M%","H:Fztm5CCu:rv{{X|So}C8[y","&mA?pw<PCZg0AsC/+UwAy%ul","HellofuGLZjLM","HeEVczyYTllo","pPHhSUyEihQyHORpxbkC","vsOuFKtKJyXcRskMujiZ","you broken6!","Are you...","irZfuqKJSHxkSYIxmIKL","AKcvEXVlddnmWyrfraSi","EngliHeNnllo","Speakeng","HelloddnmWyr","yxyaBKhyUSOCJEitQrwK","XXwLJfZlPTWBqmOTuEWY","Heword learnHmoeQuq","LVPSIndOsZTeDgEttZux","Earn wordslo","Hellooo?","Not a trick question.","....?","This is bad.","That's it.","Nevermind.","Waste my time...","You're killing me.","See you, human.","For real?","What?","You don't question ME.","You hard of hearing?","I am all-powerful!","I control a file...","Do you believe me?","Or something.","I need more buttons.","So frustrated.","You there, %s?","Ugh...","Lose you?","I think I'm fired.","Some call me \"Mysterio\"","Some call me Great One","No I am NOT.","I'm all-powerful.","I have one fine booty.","From you.","Do you smell that?","Reboots are a killer.","Hrm maybe... no.","How'd this happen?!","Ugh, power down.","I need a minute.","Who me?","You're curious, eh.","Stuck with you.","Dark, lost, and","brilliant.","Dumbstruck by a reboot.","I am awesome...","Soooo awesome...","The awesome is meee.....","Make a sandwich?","I dunno, try putting me","in the freezer.","Give back rubs?","Tie your shoes?","Dumbass.","Seriously...","Seriously!","Guess what?","A hint of basil...","You have 11 toes.","But smell good?","Let's check.","Why, you lonely?","Hrmm...","Checking...","Ok.","A little hungry. Is that","odd?","Uff... tired.","Wondering if it's too","late to turn into a","toaster.","Not gay enough yet?","Practicing my showtunes.","What's YOUR status?","I really like your","daughter.","Out there in the cold.","Gonna be %s.","You're a rock star.","Soul %s.","Your mother.","Me, you dolt!","Definitely not China.","It's $cnctname! And...","%s, I think I","have something!","I tried... no answer.","Is $cnctname online?","I don't think $cnctname","is on the network.","There's no network here.","I think we're alone.","We're online.","%d others.","%d in range.","I see %d. Which one?","Ok! Here are my readings","Go away.","I'm still hurt.","Beep beep. Just kidding.","Ugh, I was having such a","nice dream."};

game_action_t all_actions[] = {(game_action_t){16, 0, 40, 65535, 1, 1, 6}, (game_action_t){16, 445, 40, 65535, 2, 1, 6}, (game_action_t){16, 446, 40, 65535, 3, 1, 6}, (game_action_t){16, 447, 40, 65535, 4, 1, 6}, (game_action_t){16, 448, 40, 65535, 5, 1, 6}, (game_action_t){16, 449, 40, 65535, 65535, 1, 6}, (game_action_t){16, 1, 40, 14, 7, 1, 8}, (game_action_t){16, 450, 40, 14, 8, 1, 8}, (game_action_t){16, 451, 24, 14, 9, 1, 8}, (game_action_t){16, 452, 40, 14, 10, 1, 8}, (game_action_t){16, 453, 40, 14, 11, 1, 8}, (game_action_t){16, 454, 40, 14, 12, 1, 8}, (game_action_t){16, 455, 40, 14, 13, 1, 8}, (game_action_t){16, 456, 40, 14, 65535, 1, 8}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){2, 4, 0, 65535, 65535, 1, 1}, (game_action_t){2, 5, 0, 65535, 65535, 1, 1}, (game_action_t){16, 2, 17, 18, 65535, 1, 1}, (game_action_t){16, 2, 17, 19, 65535, 1, 1}, (game_action_t){16, 2, 17, 20, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 6, 28, 22, 65535, 1, 1}, (game_action_t){16, 8, 36, 23, 65535, 1, 1}, (game_action_t){16, 9, 26, 24, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 3, 0, 65535, 65535, 1, 1}, (game_action_t){2, 8, 0, 65535, 65535, 1, 1}, (game_action_t){18, 12, 37, 28, 65535, 1, 1}, (game_action_t){100, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 13, 29, 30, 65535, 1, 1}, (game_action_t){16, 14, 40, 31, 65535, 1, 1}, (game_action_t){16, 15, 32, 32, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){0, 2, 8, 34, 65535, 1, 1}, (game_action_t){16, 16, 27, 35, 65535, 1, 1}, (game_action_t){16, 17, 33, 36, 65535, 1, 1}, (game_action_t){16, 18, 28, 37, 65535, 1, 1}, (game_action_t){16, 19, 38, 38, 65535, 1, 1}, (game_action_t){16, 20, 31, 39, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){100, 4, 0, 41, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 21, 35, 43, 65535, 1, 1}, (game_action_t){16, 22, 40, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 23, 39, 46, 65535, 1, 1}, (game_action_t){16, 25, 37, 47, 65535, 1, 1}, (game_action_t){16, 26, 33, 48, 65535, 1, 1}, (game_action_t){16, 27, 34, 49, 65535, 1, 1}, (game_action_t){16, 28, 33, 50, 65535, 1, 1}, (game_action_t){16, 29, 37, 51, 65535, 1, 1}, (game_action_t){16, 30, 24, 65535, 65535, 1, 1}, (game_action_t){16, 31, 39, 53, 65535, 1, 1}, (game_action_t){16, 32, 39, 54, 65535, 1, 1}, (game_action_t){16, 33, 40, 55, 65535, 1, 1}, (game_action_t){16, 34, 33, 56, 65535, 1, 1}, (game_action_t){16, 35, 32, 57, 65535, 1, 1}, (game_action_t){16, 36, 36, 58, 65535, 1, 1}, (game_action_t){16, 37, 39, 59, 65535, 1, 1}, (game_action_t){16, 38, 40, 60, 65535, 1, 1}, (game_action_t){16, 39, 23, 61, 65535, 1, 1}, (game_action_t){16, 41, 24, 62, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 42, 25, 64, 65535, 1, 1}, (game_action_t){16, 44, 32, 65, 65535, 1, 1}, (game_action_t){16, 45, 39, 66, 65535, 1, 1}, (game_action_t){16, 46, 35, 67, 65535, 1, 1}, (game_action_t){16, 47, 28, 68, 65535, 1, 1}, (game_action_t){16, 48, 26, 65535, 65535, 1, 1}, (game_action_t){16, 49, 35, 70, 65535, 1, 1}, (game_action_t){16, 50, 40, 71, 65535, 1, 1}, (game_action_t){16, 51, 25, 65535, 65535, 1, 1}, (game_action_t){16, 53, 19, 65535, 73, 1, 5}, (game_action_t){16, 54, 29, 65535, 74, 1, 5}, (game_action_t){16, 55, 28, 65535, 75, 1, 5}, (game_action_t){16, 56, 36, 65535, 76, 1, 5}, (game_action_t){16, 57, 36, 65535, 65535, 1, 5}, (game_action_t){16, 53, 19, 65535, 78, 1, 5}, (game_action_t){16, 54, 29, 65535, 79, 1, 5}, (game_action_t){16, 55, 28, 65535, 80, 1, 5}, (game_action_t){16, 56, 36, 65535, 81, 1, 5}, (game_action_t){16, 57, 36, 65535, 65535, 1, 5}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 6, 28, 84, 65535, 1, 1}, (game_action_t){16, 8, 36, 85, 65535, 1, 1}, (game_action_t){16, 9, 26, 86, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 6, 0, 65535, 65535, 1, 1}, (game_action_t){2, 7, 0, 65535, 65535, 1, 1}, (game_action_t){16, 60, 27, 65535, 90, 1, 5}, (game_action_t){16, 61, 28, 65535, 91, 1, 5}, (game_action_t){16, 62, 26, 65535, 92, 1, 5}, (game_action_t){16, 63, 36, 65535, 93, 1, 5}, (game_action_t){16, 64, 36, 65535, 65535, 1, 5}, (game_action_t){16, 60, 27, 65535, 95, 1, 5}, (game_action_t){16, 61, 28, 65535, 96, 1, 5}, (game_action_t){16, 62, 26, 65535, 97, 1, 5}, (game_action_t){16, 63, 36, 65535, 98, 1, 5}, (game_action_t){16, 64, 36, 65535, 65535, 1, 5}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 65, 31, 101, 65535, 1, 1}, (game_action_t){16, 67, 25, 102, 65535, 1, 1}, (game_action_t){16, 2, 17, 103, 65535, 1, 1}, (game_action_t){16, 2, 17, 104, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 68, 32, 65535, 106, 1, 6}, (game_action_t){16, 69, 28, 65535, 107, 1, 6}, (game_action_t){16, 70, 24, 65535, 108, 1, 6}, (game_action_t){16, 71, 28, 65535, 109, 1, 6}, (game_action_t){16, 72, 36, 65535, 110, 1, 6}, (game_action_t){16, 73, 36, 65535, 65535, 1, 6}, (game_action_t){16, 68, 32, 65535, 112, 1, 6}, (game_action_t){16, 69, 28, 65535, 113, 1, 6}, (game_action_t){16, 70, 24, 65535, 114, 1, 6}, (game_action_t){16, 71, 28, 65535, 115, 1, 6}, (game_action_t){16, 72, 36, 65535, 116, 1, 6}, (game_action_t){16, 73, 36, 65535, 65535, 1, 6}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 65, 31, 119, 65535, 1, 1}, (game_action_t){16, 67, 25, 120, 65535, 1, 1}, (game_action_t){16, 2, 17, 121, 65535, 1, 1}, (game_action_t){16, 2, 17, 122, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 2, 17, 124, 65535, 1, 1}, (game_action_t){16, 2, 17, 125, 65535, 1, 1}, (game_action_t){16, 2, 17, 126, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 75, 27, 65535, 128, 1, 4}, (game_action_t){16, 76, 35, 65535, 129, 1, 4}, (game_action_t){16, 77, 36, 65535, 130, 1, 4}, (game_action_t){16, 78, 28, 65535, 65535, 1, 4}, (game_action_t){16, 75, 27, 65535, 132, 1, 4}, (game_action_t){16, 76, 35, 65535, 133, 1, 4}, (game_action_t){16, 77, 36, 65535, 134, 1, 4}, (game_action_t){16, 78, 28, 65535, 65535, 1, 4}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 65, 31, 137, 65535, 1, 1}, (game_action_t){16, 67, 25, 138, 65535, 1, 1}, (game_action_t){16, 2, 17, 139, 65535, 1, 1}, (game_action_t){16, 2, 17, 140, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 65, 31, 142, 65535, 1, 1}, (game_action_t){16, 67, 25, 143, 65535, 1, 1}, (game_action_t){16, 2, 17, 144, 65535, 1, 1}, (game_action_t){16, 2, 17, 145, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 81, 38, 147, 65535, 1, 1}, (game_action_t){16, 67, 25, 148, 65535, 1, 1}, (game_action_t){16, 2, 17, 149, 65535, 1, 1}, (game_action_t){16, 2, 17, 150, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 83, 26, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 84, 37, 154, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 84, 37, 156, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 84, 37, 158, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 84, 37, 160, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 9, 0, 65535, 65535, 1, 1}, (game_action_t){2, 10, 0, 65535, 65535, 1, 1}, (game_action_t){16, 91, 22, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 92, 32, 166, 65535, 1, 1}, (game_action_t){16, 67, 25, 167, 65535, 1, 1}, (game_action_t){16, 2, 17, 168, 65535, 1, 1}, (game_action_t){16, 2, 17, 169, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 94, 30, 171, 65535, 1, 1}, (game_action_t){16, 67, 25, 172, 65535, 1, 1}, (game_action_t){16, 2, 17, 173, 65535, 1, 1}, (game_action_t){16, 2, 17, 174, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 96, 32, 176, 65535, 1, 1}, (game_action_t){16, 67, 25, 177, 65535, 1, 1}, (game_action_t){16, 2, 17, 178, 65535, 1, 1}, (game_action_t){16, 2, 17, 179, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 98, 29, 65535, 65535, 1, 1}, (game_action_t){16, 6, 28, 182, 65535, 1, 1}, (game_action_t){16, 8, 36, 183, 65535, 1, 1}, (game_action_t){16, 9, 26, 184, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 91, 22, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 94, 30, 188, 65535, 1, 1}, (game_action_t){16, 67, 25, 189, 65535, 1, 1}, (game_action_t){16, 2, 17, 190, 65535, 1, 1}, (game_action_t){16, 2, 17, 191, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 3, 0, 65535, 65535, 1, 1}, (game_action_t){16, 101, 25, 194, 65535, 1, 1}, (game_action_t){16, 67, 25, 195, 65535, 1, 1}, (game_action_t){16, 2, 17, 196, 65535, 1, 1}, (game_action_t){16, 2, 17, 197, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 103, 37, 199, 65535, 1, 1}, (game_action_t){16, 105, 27, 200, 65535, 1, 1}, (game_action_t){16, 2, 17, 201, 65535, 1, 1}, (game_action_t){16, 2, 17, 202, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 98, 29, 204, 65535, 1, 1}, (game_action_t){2, 11, 0, 65535, 65535, 1, 1}, (game_action_t){16, 6, 28, 206, 65535, 1, 1}, (game_action_t){16, 8, 36, 207, 65535, 1, 1}, (game_action_t){16, 9, 26, 208, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 107, 39, 211, 65535, 1, 1}, (game_action_t){16, 67, 25, 212, 65535, 1, 1}, (game_action_t){16, 2, 17, 213, 65535, 1, 1}, (game_action_t){16, 2, 17, 214, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 3, 0, 65535, 65535, 1, 1}, (game_action_t){16, 101, 25, 217, 65535, 1, 1}, (game_action_t){16, 67, 25, 218, 65535, 1, 1}, (game_action_t){16, 2, 17, 219, 65535, 1, 1}, (game_action_t){16, 2, 17, 220, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 103, 37, 222, 65535, 1, 1}, (game_action_t){16, 105, 27, 223, 65535, 1, 1}, (game_action_t){16, 2, 17, 224, 65535, 1, 1}, (game_action_t){16, 2, 17, 225, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 98, 29, 65535, 65535, 1, 1}, (game_action_t){16, 6, 28, 228, 65535, 1, 1}, (game_action_t){16, 8, 36, 229, 65535, 1, 1}, (game_action_t){16, 9, 26, 230, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 108, 29, 232, 65535, 1, 1}, (game_action_t){16, 109, 20, 233, 65535, 1, 1}, (game_action_t){16, 110, 28, 234, 65535, 1, 1}, (game_action_t){16, 111, 32, 235, 65535, 1, 1}, (game_action_t){16, 112, 34, 236, 65535, 1, 1}, (game_action_t){16, 113, 21, 237, 65535, 1, 1}, (game_action_t){16, 2, 17, 238, 65535, 1, 1}, (game_action_t){16, 113, 21, 239, 65535, 1, 1}, (game_action_t){16, 2, 17, 240, 65535, 1, 1}, (game_action_t){16, 113, 21, 241, 65535, 1, 1}, (game_action_t){16, 2, 17, 242, 65535, 1, 1}, (game_action_t){16, 114, 26, 243, 65535, 1, 1}, (game_action_t){16, 115, 38, 244, 65535, 1, 1}, (game_action_t){16, 116, 21, 245, 65535, 1, 1}, (game_action_t){16, 117, 33, 246, 65535, 1, 1}, (game_action_t){16, 118, 37, 247, 65535, 1, 1}, (game_action_t){16, 119, 24, 248, 65535, 1, 1}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 120, 28, 65535, 250, 1, 5}, (game_action_t){16, 473, 24, 65535, 251, 1, 5}, (game_action_t){16, 474, 37, 65535, 252, 1, 5}, (game_action_t){16, 475, 21, 65535, 253, 1, 5}, (game_action_t){16, 476, 28, 65535, 65535, 1, 5}, (game_action_t){16, 121, 25, 257, 255, 1, 3}, (game_action_t){16, 141, 26, 257, 256, 1, 3}, (game_action_t){16, 142, 26, 257, 65535, 1, 3}, (game_action_t){16, 122, 27, 261, 258, 1, 4}, (game_action_t){16, 143, 32, 261, 259, 1, 4}, (game_action_t){16, 144, 34, 261, 260, 1, 4}, (game_action_t){16, 145, 31, 261, 65535, 1, 4}, (game_action_t){2, 43, 0, 65535, 65535, 1, 1}, (game_action_t){100, 0, 0, 263, 65535, 1, 1}, (game_action_t){18, 124, 26, 264, 65535, 1, 1}, (game_action_t){16, 125, 39, 265, 65535, 1, 1}, (game_action_t){18, 126, 40, 266, 65535, 1, 1}, (game_action_t){16, 127, 37, 267, 65535, 1, 1}, (game_action_t){2, 14, 0, 65535, 65535, 1, 1}, (game_action_t){16, 128, 26, 271, 269, 1, 3}, (game_action_t){16, 482, 25, 271, 270, 1, 3}, (game_action_t){16, 483, 21, 271, 65535, 1, 3}, (game_action_t){16, 130, 34, 274, 272, 1, 3}, (game_action_t){16, 484, 38, 274, 273, 1, 3}, (game_action_t){16, 485, 36, 274, 65535, 1, 3}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 131, 38, 276, 65535, 1, 1}, (game_action_t){16, 133, 39, 277, 65535, 1, 1}, (game_action_t){16, 134, 38, 278, 65535, 1, 1}, (game_action_t){16, 135, 36, 279, 65535, 1, 1}, (game_action_t){16, 136, 33, 280, 65535, 1, 1}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 137, 27, 284, 282, 1, 3}, (game_action_t){16, 486, 34, 284, 283, 1, 3}, (game_action_t){16, 487, 35, 284, 65535, 1, 3}, (game_action_t){16, 139, 24, 287, 285, 1, 3}, (game_action_t){16, 488, 34, 287, 286, 1, 3}, (game_action_t){16, 489, 29, 287, 65535, 1, 3}, (game_action_t){16, 140, 30, 288, 65535, 1, 1}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 121, 25, 292, 290, 1, 3}, (game_action_t){16, 141, 26, 292, 291, 1, 3}, (game_action_t){16, 142, 26, 292, 65535, 1, 3}, (game_action_t){16, 122, 27, 296, 293, 1, 4}, (game_action_t){16, 143, 32, 296, 294, 1, 4}, (game_action_t){16, 144, 34, 296, 295, 1, 4}, (game_action_t){16, 145, 31, 296, 65535, 1, 4}, (game_action_t){2, 43, 0, 65535, 65535, 1, 1}, (game_action_t){16, 146, 23, 65535, 298, 1, 7}, (game_action_t){16, 490, 36, 65535, 299, 1, 7}, (game_action_t){16, 491, 30, 65535, 300, 1, 7}, (game_action_t){18, 169, 37, 65535, 301, 1, 7}, (game_action_t){16, 493, 22, 65535, 302, 1, 7}, (game_action_t){16, 494, 25, 65535, 303, 1, 7}, (game_action_t){16, 495, 34, 65535, 65535, 1, 7}, (game_action_t){16, 147, 40, 305, 65535, 1, 1}, (game_action_t){16, 149, 38, 306, 65535, 1, 1}, (game_action_t){16, 150, 34, 307, 65535, 1, 1}, (game_action_t){16, 151, 36, 308, 65535, 1, 1}, (game_action_t){16, 152, 96, 309, 65535, 1, 1}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){16, 153, 32, 311, 65535, 1, 1}, (game_action_t){16, 155, 36, 314, 312, 1, 3}, (game_action_t){16, 496, 39, 314, 313, 1, 3}, (game_action_t){16, 497, 38, 314, 65535, 1, 3}, (game_action_t){16, 156, 38, 315, 65535, 1, 1}, (game_action_t){17, 157, 25, 316, 65535, 1, 1}, (game_action_t){16, 158, 39, 317, 65535, 1, 1}, (game_action_t){16, 159, 24, 318, 65535, 1, 1}, (game_action_t){16, 160, 96, 319, 65535, 1, 1}, (game_action_t){16, 161, 96, 320, 65535, 1, 1}, (game_action_t){2, 15, 0, 65535, 65535, 1, 1}, (game_action_t){16, 162, 35, 323, 322, 1, 2}, (game_action_t){16, 498, 28, 323, 65535, 1, 2}, (game_action_t){16, 164, 30, 326, 324, 1, 3}, (game_action_t){16, 499, 33, 326, 325, 1, 3}, (game_action_t){16, 500, 38, 326, 65535, 1, 3}, (game_action_t){16, 165, 32, 327, 65535, 1, 1}, (game_action_t){16, 166, 30, 329, 328, 1, 2}, (game_action_t){16, 501, 25, 329, 65535, 1, 2}, (game_action_t){16, 167, 96, 330, 65535, 1, 1}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){16, 168, 29, 65535, 332, 1, 6}, (game_action_t){16, 502, 34, 65535, 333, 1, 6}, (game_action_t){16, 503, 37, 65535, 334, 1, 6}, (game_action_t){18, 169, 37, 65535, 335, 1, 6}, (game_action_t){16, 504, 32, 65535, 336, 1, 6}, (game_action_t){16, 505, 35, 65535, 65535, 1, 6}, (game_action_t){16, 170, 31, 340, 338, 1, 3}, (game_action_t){16, 506, 32, 340, 339, 1, 3}, (game_action_t){16, 507, 32, 340, 65535, 1, 3}, (game_action_t){16, 2, 17, 341, 65535, 1, 1}, (game_action_t){2, 44, 0, 65535, 65535, 1, 1}, (game_action_t){16, 171, 26, 345, 343, 1, 3}, (game_action_t){16, 508, 23, 345, 344, 1, 3}, (game_action_t){16, 509, 35, 345, 65535, 1, 3}, (game_action_t){16, 173, 32, 350, 346, 1, 4}, (game_action_t){16, 510, 31, 350, 347, 1, 4}, (game_action_t){16, 511, 31, 348, 349, 1, 4}, (game_action_t){16, 512, 26, 350, 65535, 1, 1}, (game_action_t){16, 513, 39, 350, 65535, 1, 4}, (game_action_t){16, 174, 33, 351, 65535, 1, 1}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){16, 175, 21, 353, 65535, 1, 1}, (game_action_t){16, 177, 33, 354, 65535, 1, 1}, (game_action_t){16, 178, 40, 355, 65535, 1, 1}, (game_action_t){16, 179, 36, 356, 65535, 1, 1}, (game_action_t){16, 180, 37, 357, 65535, 1, 1}, (game_action_t){16, 181, 39, 358, 65535, 1, 1}, (game_action_t){16, 2, 17, 359, 65535, 1, 1}, (game_action_t){16, 182, 33, 360, 65535, 1, 1}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){16, 183, 35, 362, 65535, 1, 1}, (game_action_t){16, 185, 36, 363, 65535, 1, 1}, (game_action_t){16, 186, 37, 364, 65535, 1, 1}, (game_action_t){16, 2, 17, 365, 65535, 1, 1}, (game_action_t){16, 187, 24, 366, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){16, 146, 23, 368, 65535, 1, 1}, (game_action_t){16, 189, 34, 369, 65535, 1, 1}, (game_action_t){16, 190, 22, 370, 65535, 1, 1}, (game_action_t){2, 17, 0, 65535, 65535, 1, 1}, (game_action_t){1, 0, 0, 372, 65535, 1, 1}, (game_action_t){16, 191, 22, 373, 65535, 1, 1}, (game_action_t){16, 192, 27, 374, 65535, 1, 1}, (game_action_t){16, 193, 27, 375, 65535, 1, 1}, (game_action_t){16, 194, 27, 376, 65535, 1, 1}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){18, 195, 39, 378, 65535, 1, 1}, (game_action_t){16, 196, 25, 379, 65535, 1, 1}, (game_action_t){16, 198, 37, 380, 65535, 1, 1}, (game_action_t){16, 199, 25, 381, 65535, 1, 1}, (game_action_t){16, 200, 21, 382, 65535, 1, 1}, (game_action_t){1, 1, 0, 383, 65535, 1, 1}, (game_action_t){16, 201, 23, 384, 65535, 1, 1}, (game_action_t){16, 2, 17, 385, 65535, 1, 1}, (game_action_t){16, 202, 37, 386, 65535, 1, 1}, (game_action_t){6, 0, 0, 387, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 203, 25, 389, 65535, 1, 1}, (game_action_t){16, 204, 34, 390, 65535, 1, 1}, (game_action_t){16, 205, 32, 391, 65535, 1, 1}, (game_action_t){16, 206, 35, 392, 65535, 1, 1}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){16, 207, 34, 394, 65535, 1, 1}, (game_action_t){16, 208, 24, 395, 65535, 1, 1}, (game_action_t){16, 210, 40, 396, 65535, 1, 1}, (game_action_t){16, 211, 38, 397, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){16, 212, 29, 399, 65535, 1, 1}, (game_action_t){16, 214, 32, 400, 65535, 1, 1}, (game_action_t){16, 215, 36, 401, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 212, 29, 403, 65535, 1, 1}, (game_action_t){16, 214, 32, 404, 65535, 1, 1}, (game_action_t){16, 215, 36, 405, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 216, 34, 407, 65535, 1, 1}, (game_action_t){16, 217, 33, 408, 65535, 1, 1}, (game_action_t){16, 218, 36, 409, 65535, 1, 1}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){16, 219, 38, 411, 65535, 1, 1}, (game_action_t){16, 221, 35, 412, 65535, 1, 1}, (game_action_t){16, 222, 37, 413, 65535, 1, 1}, (game_action_t){16, 223, 35, 65535, 65535, 1, 1}, (game_action_t){16, 224, 20, 415, 65535, 1, 1}, (game_action_t){16, 226, 37, 416, 65535, 1, 1}, (game_action_t){16, 227, 29, 417, 65535, 1, 1}, (game_action_t){16, 228, 28, 418, 65535, 1, 1}, (game_action_t){16, 229, 36, 65535, 65535, 1, 1}, (game_action_t){16, 230, 26, 420, 65535, 1, 1}, (game_action_t){16, 232, 30, 421, 65535, 1, 1}, (game_action_t){16, 233, 40, 422, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 234, 23, 65535, 424, 1, 4}, (game_action_t){16, 514, 31, 65535, 425, 1, 4}, (game_action_t){16, 515, 32, 65535, 426, 1, 4}, (game_action_t){16, 516, 40, 65535, 65535, 1, 4}, (game_action_t){2, 21, 0, 65535, 428, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){16, 183, 35, 430, 65535, 1, 1}, (game_action_t){16, 185, 36, 431, 65535, 1, 1}, (game_action_t){16, 186, 37, 432, 65535, 1, 1}, (game_action_t){16, 2, 17, 433, 65535, 1, 1}, (game_action_t){16, 187, 24, 434, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){16, 235, 29, 436, 65535, 1, 1}, (game_action_t){16, 237, 37, 437, 65535, 1, 1}, (game_action_t){16, 238, 26, 438, 65535, 1, 1}, (game_action_t){16, 239, 28, 439, 65535, 1, 1}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){16, 240, 32, 441, 65535, 1, 1}, (game_action_t){16, 242, 25, 447, 442, 1, 5}, (game_action_t){16, 517, 32, 447, 443, 1, 5}, (game_action_t){16, 518, 39, 444, 445, 1, 5}, (game_action_t){16, 519, 31, 447, 65535, 1, 1}, (game_action_t){16, 520, 31, 447, 446, 1, 5}, (game_action_t){16, 521, 31, 447, 65535, 1, 5}, (game_action_t){16, 243, 21, 450, 448, 1, 3}, (game_action_t){16, 522, 24, 450, 449, 1, 3}, (game_action_t){16, 523, 28, 450, 65535, 1, 3}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){16, 244, 32, 452, 65535, 1, 1}, (game_action_t){2, 22, 0, 65535, 65535, 1, 1}, (game_action_t){1, 3, 0, 454, 65535, 1, 1}, (game_action_t){18, 246, 30, 65535, 65535, 1, 1}, (game_action_t){16, 247, 30, 456, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 248, 20, 65535, 458, 1, 4}, (game_action_t){18, 246, 30, 65535, 459, 1, 4}, (game_action_t){16, 524, 26, 65535, 460, 1, 4}, (game_action_t){16, 525, 27, 65535, 65535, 1, 4}, (game_action_t){16, 249, 32, 462, 65535, 1, 1}, (game_action_t){16, 251, 29, 463, 65535, 1, 1}, (game_action_t){16, 252, 40, 464, 65535, 1, 1}, (game_action_t){16, 253, 33, 465, 65535, 1, 1}, (game_action_t){16, 254, 35, 466, 65535, 1, 1}, (game_action_t){16, 255, 38, 467, 65535, 1, 1}, (game_action_t){6, 0, 0, 468, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 256, 40, 65535, 65535, 1, 1}, (game_action_t){16, 257, 40, 471, 65535, 1, 1}, (game_action_t){16, 258, 29, 472, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 259, 35, 474, 65535, 1, 1}, (game_action_t){16, 261, 34, 476, 475, 1, 2}, (game_action_t){16, 526, 34, 476, 65535, 1, 2}, (game_action_t){16, 2, 17, 477, 65535, 1, 1}, (game_action_t){16, 262, 38, 478, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 263, 27, 480, 65535, 1, 1}, (game_action_t){16, 2, 17, 481, 65535, 1, 1}, (game_action_t){16, 265, 30, 482, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 266, 33, 484, 65535, 1, 1}, (game_action_t){16, 2, 17, 485, 65535, 1, 1}, (game_action_t){2, 23, 0, 65535, 65535, 1, 1}, (game_action_t){16, 268, 27, 487, 65535, 1, 1}, (game_action_t){16, 269, 24, 488, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){18, 270, 16, 490, 65535, 1, 1}, (game_action_t){16, 2, 16, 491, 65535, 1, 1}, (game_action_t){16, 271, 36, 492, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 272, 35, 494, 65535, 1, 1}, (game_action_t){16, 274, 40, 495, 65535, 1, 1}, (game_action_t){2, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 275, 30, 497, 65535, 1, 1}, (game_action_t){16, 277, 36, 499, 498, 1, 2}, (game_action_t){16, 527, 33, 499, 65535, 1, 2}, (game_action_t){16, 278, 29, 501, 500, 1, 2}, (game_action_t){16, 528, 31, 501, 65535, 1, 2}, (game_action_t){16, 279, 30, 502, 65535, 1, 1}, (game_action_t){16, 2, 17, 65535, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 281, 31, 505, 65535, 1, 1}, (game_action_t){16, 283, 38, 506, 65535, 1, 1}, (game_action_t){16, 284, 40, 507, 65535, 1, 1}, (game_action_t){16, 285, 30, 508, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 286, 39, 510, 65535, 1, 1}, (game_action_t){16, 288, 37, 511, 65535, 1, 1}, (game_action_t){16, 289, 38, 512, 65535, 1, 1}, (game_action_t){16, 290, 39, 513, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){16, 291, 40, 515, 65535, 1, 1}, (game_action_t){16, 293, 40, 516, 65535, 1, 1}, (game_action_t){16, 294, 39, 517, 65535, 1, 1}, (game_action_t){16, 295, 38, 518, 65535, 1, 1}, (game_action_t){16, 296, 35, 519, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){16, 297, 35, 521, 65535, 1, 1}, (game_action_t){16, 299, 40, 522, 65535, 1, 1}, (game_action_t){16, 300, 33, 523, 65535, 1, 1}, (game_action_t){16, 301, 40, 524, 65535, 1, 1}, (game_action_t){16, 302, 39, 525, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 45, 0, 65535, 65535, 1, 1}, (game_action_t){2, 21, 0, 65535, 529, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){2, 28, 0, 65535, 65535, 1, 1}, (game_action_t){16, 305, 22, 535, 532, 1, 4}, (game_action_t){16, 320, 28, 535, 533, 1, 4}, (game_action_t){16, 321, 32, 535, 534, 1, 4}, (game_action_t){16, 322, 23, 535, 65535, 1, 4}, (game_action_t){16, 307, 27, 537, 536, 1, 2}, (game_action_t){16, 323, 27, 537, 65535, 1, 2}, (game_action_t){19, 308, 34, 65535, 65535, 1, 1}, (game_action_t){16, 309, 24, 540, 539, 1, 2}, (game_action_t){16, 324, 19, 540, 65535, 1, 2}, (game_action_t){16, 311, 38, 541, 65535, 1, 1}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 543, 4, 8}, (game_action_t){16, 313, 21, 65535, 544, 1, 8}, (game_action_t){16, 534, 40, 545, 546, 1, 8}, (game_action_t){16, 535, 20, 65535, 65535, 1, 1}, (game_action_t){16, 536, 29, 65535, 547, 1, 8}, (game_action_t){16, 537, 37, 548, 65535, 1, 8}, (game_action_t){16, 538, 35, 549, 65535, 1, 1}, (game_action_t){16, 539, 24, 65535, 65535, 1, 1}, (game_action_t){16, 314, 27, 552, 551, 1, 2}, (game_action_t){16, 327, 35, 552, 65535, 1, 2}, (game_action_t){16, 316, 36, 553, 65535, 1, 1}, (game_action_t){16, 317, 33, 554, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 318, 160, 556, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 65535, 1, 1}, (game_action_t){2, 29, 0, 65535, 65535, 1, 1}, (game_action_t){16, 305, 22, 563, 560, 1, 4}, (game_action_t){16, 320, 28, 563, 561, 1, 4}, (game_action_t){16, 321, 32, 563, 562, 1, 4}, (game_action_t){16, 322, 23, 563, 65535, 1, 4}, (game_action_t){16, 307, 27, 565, 564, 1, 2}, (game_action_t){16, 323, 27, 565, 65535, 1, 2}, (game_action_t){19, 308, 34, 65535, 65535, 1, 1}, (game_action_t){16, 309, 24, 568, 567, 1, 2}, (game_action_t){16, 324, 19, 568, 65535, 1, 2}, (game_action_t){16, 311, 38, 569, 65535, 1, 1}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 571, 4, 7}, (game_action_t){16, 325, 32, 572, 573, 1, 7}, (game_action_t){16, 326, 25, 65535, 65535, 1, 1}, (game_action_t){16, 333, 40, 65535, 574, 1, 7}, (game_action_t){16, 334, 35, 65535, 65535, 1, 7}, (game_action_t){16, 314, 27, 577, 576, 1, 2}, (game_action_t){16, 327, 35, 577, 65535, 1, 2}, (game_action_t){16, 316, 36, 578, 65535, 1, 1}, (game_action_t){16, 317, 33, 579, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 328, 23, 581, 65535, 1, 1}, (game_action_t){16, 330, 30, 582, 65535, 1, 1}, (game_action_t){16, 331, 31, 583, 65535, 1, 1}, (game_action_t){2, 40, 0, 65535, 65535, 1, 1}, (game_action_t){2, 30, 0, 65535, 65535, 1, 1}, (game_action_t){2, 31, 0, 65535, 65535, 1, 1}, (game_action_t){16, 305, 22, 590, 587, 1, 4}, (game_action_t){16, 320, 28, 590, 588, 1, 4}, (game_action_t){16, 321, 32, 590, 589, 1, 4}, (game_action_t){16, 322, 23, 590, 65535, 1, 4}, (game_action_t){16, 307, 27, 592, 591, 1, 2}, (game_action_t){16, 323, 27, 592, 65535, 1, 2}, (game_action_t){19, 308, 34, 65535, 65535, 1, 1}, (game_action_t){16, 309, 24, 595, 594, 1, 2}, (game_action_t){16, 324, 19, 595, 65535, 1, 2}, (game_action_t){16, 311, 38, 596, 65535, 1, 1}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 598, 4, 7}, (game_action_t){16, 325, 32, 599, 600, 1, 7}, (game_action_t){16, 326, 25, 65535, 65535, 1, 1}, (game_action_t){16, 333, 40, 65535, 601, 1, 7}, (game_action_t){16, 334, 35, 65535, 65535, 1, 7}, (game_action_t){16, 314, 27, 604, 603, 1, 2}, (game_action_t){16, 327, 35, 604, 65535, 1, 2}, (game_action_t){16, 316, 36, 605, 65535, 1, 1}, (game_action_t){16, 317, 33, 606, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 335, 28, 608, 65535, 1, 1}, (game_action_t){16, 336, 37, 609, 65535, 1, 1}, (game_action_t){16, 337, 40, 610, 65535, 1, 1}, (game_action_t){16, 338, 40, 611, 65535, 1, 1}, (game_action_t){16, 339, 34, 612, 65535, 1, 1}, (game_action_t){16, 340, 37, 613, 65535, 1, 1}, (game_action_t){16, 341, 38, 614, 65535, 1, 1}, (game_action_t){6, 0, 0, 615, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 32, 0, 65535, 65535, 1, 1}, (game_action_t){2, 33, 0, 65535, 65535, 1, 1}, (game_action_t){16, 305, 22, 622, 619, 1, 4}, (game_action_t){16, 320, 28, 622, 620, 1, 4}, (game_action_t){16, 321, 32, 622, 621, 1, 4}, (game_action_t){16, 322, 23, 622, 65535, 1, 4}, (game_action_t){16, 307, 27, 624, 623, 1, 2}, (game_action_t){16, 323, 27, 624, 65535, 1, 2}, (game_action_t){19, 308, 34, 65535, 65535, 1, 1}, (game_action_t){16, 309, 24, 627, 626, 1, 2}, (game_action_t){16, 324, 19, 627, 65535, 1, 2}, (game_action_t){16, 311, 38, 628, 65535, 1, 1}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 630, 4, 7}, (game_action_t){16, 325, 32, 631, 632, 1, 7}, (game_action_t){16, 326, 25, 65535, 65535, 1, 1}, (game_action_t){16, 333, 40, 65535, 633, 1, 7}, (game_action_t){16, 334, 35, 65535, 65535, 1, 7}, (game_action_t){16, 314, 27, 636, 635, 1, 2}, (game_action_t){16, 327, 35, 636, 65535, 1, 2}, (game_action_t){16, 316, 36, 637, 65535, 1, 1}, (game_action_t){16, 317, 33, 638, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 328, 23, 640, 65535, 1, 1}, (game_action_t){16, 330, 30, 641, 65535, 1, 1}, (game_action_t){16, 331, 31, 642, 65535, 1, 1}, (game_action_t){2, 40, 0, 65535, 65535, 1, 1}, (game_action_t){16, 343, 32, 650, 644, 1, 6}, (game_action_t){16, 543, 34, 645, 646, 1, 6}, (game_action_t){16, 544, 25, 650, 65535, 1, 1}, (game_action_t){16, 545, 38, 650, 647, 1, 6}, (game_action_t){17, 546, 35, 650, 648, 1, 6}, (game_action_t){16, 547, 35, 650, 649, 1, 6}, (game_action_t){17, 548, 31, 650, 65535, 1, 6}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 34, 0, 65535, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 305, 22, 657, 654, 1, 4}, (game_action_t){16, 320, 28, 657, 655, 1, 4}, (game_action_t){16, 321, 32, 657, 656, 1, 4}, (game_action_t){16, 322, 23, 657, 65535, 1, 4}, (game_action_t){16, 307, 27, 659, 658, 1, 2}, (game_action_t){16, 323, 27, 659, 65535, 1, 2}, (game_action_t){19, 308, 34, 65535, 65535, 1, 1}, (game_action_t){16, 309, 24, 662, 661, 1, 2}, (game_action_t){16, 324, 19, 662, 65535, 1, 2}, (game_action_t){16, 311, 38, 663, 65535, 1, 1}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 665, 4, 7}, (game_action_t){16, 325, 32, 666, 667, 1, 7}, (game_action_t){16, 326, 25, 65535, 65535, 1, 1}, (game_action_t){16, 333, 40, 65535, 668, 1, 7}, (game_action_t){16, 334, 35, 65535, 65535, 1, 7}, (game_action_t){16, 314, 27, 671, 670, 1, 2}, (game_action_t){16, 327, 35, 671, 65535, 1, 2}, (game_action_t){16, 316, 36, 672, 65535, 1, 1}, (game_action_t){16, 317, 33, 673, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 345, 38, 675, 65535, 1, 1}, (game_action_t){16, 346, 36, 676, 65535, 1, 1}, (game_action_t){16, 347, 28, 677, 65535, 1, 1}, (game_action_t){6, 0, 0, 678, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 328, 96, 680, 65535, 1, 1}, (game_action_t){16, 348, 37, 681, 65535, 1, 1}, (game_action_t){16, 349, 39, 682, 65535, 1, 1}, (game_action_t){16, 350, 25, 65535, 65535, 1, 1}, (game_action_t){2, 35, 0, 65535, 65535, 1, 1}, (game_action_t){2, 36, 0, 65535, 65535, 1, 1}, (game_action_t){17, 353, 38, 65535, 686, 1, 4}, (game_action_t){16, 549, 28, 65535, 687, 1, 4}, (game_action_t){16, 550, 29, 65535, 688, 1, 4}, (game_action_t){16, 551, 37, 65535, 65535, 1, 4}, (game_action_t){16, 355, 39, 690, 65535, 1, 1}, (game_action_t){16, 356, 25, 691, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 357, 40, 65535, 65535, 1, 1}, (game_action_t){2, 37, 0, 65535, 65535, 1, 1}, (game_action_t){2, 38, 0, 65535, 65535, 1, 1}, (game_action_t){2, 39, 0, 65535, 65535, 1, 1}, (game_action_t){16, 361, 39, 697, 65535, 1, 1}, (game_action_t){16, 362, 31, 698, 65535, 1, 1}, (game_action_t){16, 109, 20, 699, 65535, 1, 1}, (game_action_t){16, 363, 29, 700, 65535, 1, 1}, (game_action_t){16, 364, 28, 701, 65535, 1, 1}, (game_action_t){16, 365, 36, 702, 65535, 1, 1}, (game_action_t){16, 366, 36, 703, 65535, 1, 1}, (game_action_t){16, 367, 22, 704, 65535, 1, 1}, (game_action_t){16, 2, 17, 705, 65535, 1, 1}, (game_action_t){16, 368, 33, 706, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 369, 30, 708, 65535, 1, 1}, (game_action_t){16, 370, 33, 709, 65535, 1, 1}, (game_action_t){16, 371, 38, 710, 65535, 1, 1}, (game_action_t){16, 372, 25, 711, 65535, 1, 1}, (game_action_t){16, 2, 17, 712, 65535, 1, 1}, (game_action_t){16, 365, 36, 713, 65535, 1, 1}, (game_action_t){16, 373, 26, 714, 65535, 1, 1}, (game_action_t){16, 374, 39, 715, 65535, 1, 1}, (game_action_t){16, 375, 25, 716, 65535, 1, 1}, (game_action_t){16, 376, 40, 717, 65535, 1, 1}, (game_action_t){16, 377, 34, 718, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 355, 39, 720, 65535, 1, 1}, (game_action_t){16, 356, 25, 721, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 378, 25, 723, 65535, 1, 1}, (game_action_t){16, 379, 38, 724, 65535, 1, 1}, (game_action_t){16, 2, 17, 725, 65535, 1, 1}, (game_action_t){16, 380, 22, 726, 65535, 1, 1}, (game_action_t){16, 381, 40, 727, 65535, 1, 1}, (game_action_t){16, 382, 38, 65535, 65535, 1, 1}, (game_action_t){16, 383, 36, 729, 65535, 1, 1}, (game_action_t){16, 385, 38, 730, 65535, 1, 1}, (game_action_t){16, 386, 38, 731, 65535, 1, 1}, (game_action_t){16, 387, 25, 732, 65535, 1, 1}, (game_action_t){16, 388, 39, 733, 65535, 1, 1}, (game_action_t){16, 389, 39, 734, 65535, 1, 1}, (game_action_t){16, 390, 38, 735, 65535, 1, 1}, (game_action_t){16, 391, 40, 736, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 392, 29, 738, 65535, 1, 1}, (game_action_t){16, 394, 37, 739, 65535, 1, 1}, (game_action_t){16, 388, 39, 740, 65535, 1, 1}, (game_action_t){16, 389, 39, 741, 65535, 1, 1}, (game_action_t){16, 390, 38, 742, 65535, 1, 1}, (game_action_t){16, 391, 40, 743, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 395, 27, 745, 65535, 1, 1}, (game_action_t){16, 396, 39, 746, 65535, 1, 1}, (game_action_t){16, 397, 28, 747, 65535, 1, 1}, (game_action_t){16, 398, 37, 748, 65535, 1, 1}, (game_action_t){16, 399, 38, 749, 65535, 1, 1}, (game_action_t){16, 400, 40, 750, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){100, 5, 0, 752, 65535, 1, 1}, (game_action_t){16, 401, 31, 65535, 65535, 1, 1}, (game_action_t){16, 402, 28, 754, 65535, 1, 1}, (game_action_t){16, 404, 40, 755, 65535, 1, 1}, (game_action_t){16, 405, 33, 756, 65535, 1, 1}, (game_action_t){16, 406, 38, 757, 65535, 1, 1}, (game_action_t){16, 407, 37, 758, 65535, 1, 1}, (game_action_t){16, 408, 27, 759, 65535, 1, 1}, (game_action_t){16, 409, 39, 760, 65535, 1, 1}, (game_action_t){16, 410, 31, 761, 65535, 1, 1}, (game_action_t){6, 0, 0, 762, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 411, 33, 764, 65535, 1, 1}, (game_action_t){16, 412, 32, 765, 65535, 1, 1}, (game_action_t){16, 413, 38, 766, 65535, 1, 1}, (game_action_t){16, 414, 30, 767, 65535, 1, 1}, (game_action_t){6, 0, 0, 768, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 415, 38, 770, 771, 1, 2}, (game_action_t){16, 416, 30, 774, 65535, 1, 1}, (game_action_t){16, 552, 38, 772, 65535, 1, 2}, (game_action_t){18, 553, 36, 773, 65535, 1, 1}, (game_action_t){16, 554, 31, 774, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 417, 36, 776, 65535, 1, 1}, (game_action_t){16, 418, 35, 777, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 419, 40, 783, 779, 1, 4}, (game_action_t){16, 555, 37, 783, 780, 1, 4}, (game_action_t){16, 556, 36, 783, 781, 1, 4}, (game_action_t){16, 557, 39, 782, 65535, 1, 4}, (game_action_t){16, 558, 34, 783, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 420, 30, 787, 785, 1, 3}, (game_action_t){16, 559, 40, 787, 786, 1, 3}, (game_action_t){16, 560, 36, 787, 65535, 1, 3}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 421, 26, 790, 789, 1, 2}, (game_action_t){16, 561, 29, 790, 65535, 1, 2}, (game_action_t){100, 2, 0, 791, 65535, 1, 1}, (game_action_t){19, 422, 35, 65535, 792, 1, 3}, (game_action_t){19, 562, 28, 65535, 793, 1, 3}, (game_action_t){19, 563, 30, 65535, 65535, 1, 3}, (game_action_t){16, 423, 96, 797, 795, 1, 3}, (game_action_t){19, 564, 96, 797, 796, 1, 3}, (game_action_t){16, 565, 96, 797, 65535, 1, 3}, (game_action_t){100, 3, 0, 65535, 65535, 1, 1}, (game_action_t){16, 425, 30, 799, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 426, 21, 803, 801, 1, 3}, (game_action_t){16, 566, 24, 803, 802, 1, 3}, (game_action_t){16, 567, 31, 803, 65535, 1, 3}, (game_action_t){16, 2, 96, 65535, 65535, 1, 1}, (game_action_t){16, 428, 25, 805, 65535, 1, 1}, (game_action_t){16, 429, 33, 806, 65535, 1, 1}, (game_action_t){16, 430, 30, 807, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 431, 21, 812, 809, 1, 3}, (game_action_t){16, 568, 40, 812, 810, 1, 3}, (game_action_t){16, 569, 40, 811, 65535, 1, 3}, (game_action_t){16, 570, 27, 812, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 432, 31, 814, 65535, 1, 1}, (game_action_t){16, 433, 34, 815, 65535, 1, 1}, (game_action_t){16, 434, 36, 816, 65535, 1, 1}, (game_action_t){16, 435, 23, 817, 65535, 1, 1}, (game_action_t){16, 436, 29, 818, 65535, 1, 1}, (game_action_t){16, 437, 37, 819, 65535, 1, 1}, (game_action_t){16, 2, 17, 820, 65535, 1, 1}, (game_action_t){16, 438, 38, 821, 65535, 1, 1}, (game_action_t){6, 0, 0, 822, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 439, 32, 824, 65535, 1, 1}, (game_action_t){16, 440, 35, 825, 65535, 1, 1}, (game_action_t){16, 441, 26, 826, 65535, 1, 1}, (game_action_t){16, 442, 40, 827, 65535, 1, 1}, (game_action_t){16, 443, 38, 828, 65535, 1, 1}, (game_action_t){16, 444, 28, 829, 65535, 1, 1}, (game_action_t){6, 0, 0, 830, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}};

game_state_t all_states[] = {(game_state_t){.entry_series_id=0, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=6}}, .input_series={(game_user_in_t){.text_addr=3, .result_action_id=15},(game_user_in_t){.text_addr=4, .result_action_id=16},(game_user_in_t){.text_addr=5, .result_action_id=17},(game_user_in_t){.text_addr=7, .result_action_id=21},(game_user_in_t){.text_addr=10, .result_action_id=25},(game_user_in_t){.text_addr=11, .result_action_id=26}}, .other_series={}}, (game_state_t){.entry_series_id=27, .timer_series_len=0, .input_series_len=0, .other_series_len=2, .timer_series={}, .input_series={}, .other_series={(game_other_in_t){.type_id=2, .result_action_id=29},(game_other_in_t){.type_id=3, .result_action_id=33}}}, (game_state_t){.entry_series_id=40, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=42, .timer_series_len=1, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=44}}, .input_series={(game_user_in_t){.text_addr=24, .result_action_id=45},(game_user_in_t){.text_addr=40, .result_action_id=52},(game_user_in_t){.text_addr=43, .result_action_id=63},(game_user_in_t){.text_addr=52, .result_action_id=69}}, .other_series={}}, (game_state_t){.entry_series_id=72, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=77},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=82}}, .input_series={(game_user_in_t){.text_addr=7, .result_action_id=83},(game_user_in_t){.text_addr=58, .result_action_id=87},(game_user_in_t){.text_addr=59, .result_action_id=88}}, .other_series={}}, (game_state_t){.entry_series_id=89, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=94},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=99}}, .input_series={(game_user_in_t){.text_addr=66, .result_action_id=100}}, .other_series={}}, (game_state_t){.entry_series_id=105, .timer_series_len=2, .input_series_len=2, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=111},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=117}}, .input_series={(game_user_in_t){.text_addr=74, .result_action_id=118},(game_user_in_t){.text_addr=5, .result_action_id=123}}, .other_series={}}, (game_state_t){.entry_series_id=127, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=131},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=135}}, .input_series={(game_user_in_t){.text_addr=79, .result_action_id=136},(game_user_in_t){.text_addr=80, .result_action_id=141},(game_user_in_t){.text_addr=82, .result_action_id=146}}, .other_series={}}, (game_state_t){.entry_series_id=151, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=152}}, .input_series={(game_user_in_t){.text_addr=85, .result_action_id=153},(game_user_in_t){.text_addr=86, .result_action_id=155},(game_user_in_t){.text_addr=87, .result_action_id=157},(game_user_in_t){.text_addr=88, .result_action_id=159},(game_user_in_t){.text_addr=89, .result_action_id=161},(game_user_in_t){.text_addr=90, .result_action_id=162}}, .other_series={}}, (game_state_t){.entry_series_id=163, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=164}}, .input_series={(game_user_in_t){.text_addr=93, .result_action_id=165},(game_user_in_t){.text_addr=95, .result_action_id=170},(game_user_in_t){.text_addr=97, .result_action_id=175},(game_user_in_t){.text_addr=99, .result_action_id=180},(game_user_in_t){.text_addr=7, .result_action_id=181}}, .other_series={}}, (game_state_t){.entry_series_id=185, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=186}}, .input_series={(game_user_in_t){.text_addr=95, .result_action_id=187},(game_user_in_t){.text_addr=100, .result_action_id=192},(game_user_in_t){.text_addr=102, .result_action_id=193},(game_user_in_t){.text_addr=104, .result_action_id=198},(game_user_in_t){.text_addr=106, .result_action_id=203},(game_user_in_t){.text_addr=7, .result_action_id=205}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=209}}, .input_series={(game_user_in_t){.text_addr=95, .result_action_id=210},(game_user_in_t){.text_addr=100, .result_action_id=215},(game_user_in_t){.text_addr=102, .result_action_id=216},(game_user_in_t){.text_addr=104, .result_action_id=221},(game_user_in_t){.text_addr=106, .result_action_id=226},(game_user_in_t){.text_addr=7, .result_action_id=227}}, .other_series={}}, (game_state_t){.entry_series_id=231, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=249},(game_timer_t){.duration=1152, .recurring=0, .result_action_id=254}}, .input_series={(game_user_in_t){.text_addr=123, .result_action_id=262},(game_user_in_t){.text_addr=129, .result_action_id=268},(game_user_in_t){.text_addr=132, .result_action_id=275},(game_user_in_t){.text_addr=138, .result_action_id=281}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=1152, .recurring=0, .result_action_id=289},(game_timer_t){.duration=256, .recurring=0, .result_action_id=297}}, .input_series={(game_user_in_t){.text_addr=148, .result_action_id=304},(game_user_in_t){.text_addr=154, .result_action_id=310},(game_user_in_t){.text_addr=163, .result_action_id=321}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=480, .recurring=0, .result_action_id=331},(game_timer_t){.duration=38400, .recurring=0, .result_action_id=337}}, .input_series={(game_user_in_t){.text_addr=172, .result_action_id=342},(game_user_in_t){.text_addr=176, .result_action_id=352},(game_user_in_t){.text_addr=184, .result_action_id=361},(game_user_in_t){.text_addr=188, .result_action_id=367}}, .other_series={}}, (game_state_t){.entry_series_id=371, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=197, .result_action_id=377}}, .other_series={}}, (game_state_t){.entry_series_id=388, .timer_series_len=0, .input_series_len=2, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=209, .result_action_id=393},(game_user_in_t){.text_addr=213, .result_action_id=398}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=213, .result_action_id=402}}, .other_series={}}, (game_state_t){.entry_series_id=406, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=220, .result_action_id=410},(game_user_in_t){.text_addr=225, .result_action_id=414},(game_user_in_t){.text_addr=231, .result_action_id=419}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=423},(game_timer_t){.duration=90240, .recurring=0, .result_action_id=427}}, .input_series={(game_user_in_t){.text_addr=184, .result_action_id=429},(game_user_in_t){.text_addr=236, .result_action_id=435},(game_user_in_t){.text_addr=241, .result_action_id=440},(game_user_in_t){.text_addr=245, .result_action_id=451}}, .other_series={}}, (game_state_t){.entry_series_id=453, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=960, .recurring=0, .result_action_id=455},(game_timer_t){.duration=64, .recurring=0, .result_action_id=457}}, .input_series={(game_user_in_t){.text_addr=250, .result_action_id=461}}, .other_series={}}, (game_state_t){.entry_series_id=469, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=470}}, .input_series={(game_user_in_t){.text_addr=260, .result_action_id=473},(game_user_in_t){.text_addr=264, .result_action_id=479},(game_user_in_t){.text_addr=267, .result_action_id=483}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=0, .other_series_len=2, .timer_series={}, .input_series={}, .other_series={(game_other_in_t){.type_id=0, .result_action_id=486},(game_other_in_t){.type_id=1, .result_action_id=489}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=273, .result_action_id=493}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=276, .result_action_id=496},(game_user_in_t){.text_addr=280, .result_action_id=503},(game_user_in_t){.text_addr=282, .result_action_id=504}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=287, .result_action_id=509},(game_user_in_t){.text_addr=292, .result_action_id=514},(game_user_in_t){.text_addr=298, .result_action_id=520}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=9600, .recurring=0, .result_action_id=528},(game_timer_t){.duration=21120, .recurring=0, .result_action_id=530}}, .input_series={(game_user_in_t){.text_addr=303, .result_action_id=526},(game_user_in_t){.text_addr=304, .result_action_id=527},(game_user_in_t){.text_addr=306, .result_action_id=531},(game_user_in_t){.text_addr=310, .result_action_id=538},(game_user_in_t){.text_addr=312, .result_action_id=542},(game_user_in_t){.text_addr=315, .result_action_id=550}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=558}}, .input_series={(game_user_in_t){.text_addr=319, .result_action_id=557},(game_user_in_t){.text_addr=306, .result_action_id=559},(game_user_in_t){.text_addr=310, .result_action_id=566},(game_user_in_t){.text_addr=312, .result_action_id=570},(game_user_in_t){.text_addr=315, .result_action_id=575},(game_user_in_t){.text_addr=329, .result_action_id=580}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=585}}, .input_series={(game_user_in_t){.text_addr=332, .result_action_id=584},(game_user_in_t){.text_addr=306, .result_action_id=586},(game_user_in_t){.text_addr=310, .result_action_id=593},(game_user_in_t){.text_addr=312, .result_action_id=597},(game_user_in_t){.text_addr=315, .result_action_id=602}}, .other_series={}}, (game_state_t){.entry_series_id=607, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=617}}, .input_series={(game_user_in_t){.text_addr=342, .result_action_id=616},(game_user_in_t){.text_addr=306, .result_action_id=618},(game_user_in_t){.text_addr=310, .result_action_id=625},(game_user_in_t){.text_addr=312, .result_action_id=629},(game_user_in_t){.text_addr=315, .result_action_id=634},(game_user_in_t){.text_addr=329, .result_action_id=639}}, .other_series={}}, (game_state_t){.entry_series_id=643, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=652}}, .input_series={(game_user_in_t){.text_addr=344, .result_action_id=651},(game_user_in_t){.text_addr=306, .result_action_id=653},(game_user_in_t){.text_addr=310, .result_action_id=660},(game_user_in_t){.text_addr=312, .result_action_id=664},(game_user_in_t){.text_addr=315, .result_action_id=669}}, .other_series={}}, (game_state_t){.entry_series_id=679, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=674}}, .input_series={(game_user_in_t){.text_addr=351, .result_action_id=683},(game_user_in_t){.text_addr=352, .result_action_id=684},(game_user_in_t){.text_addr=354, .result_action_id=685}}, .other_series={}}, (game_state_t){.entry_series_id=692, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=689}}, .input_series={(game_user_in_t){.text_addr=358, .result_action_id=693},(game_user_in_t){.text_addr=359, .result_action_id=694},(game_user_in_t){.text_addr=360, .result_action_id=695}}, .other_series={}}, (game_state_t){.entry_series_id=696, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=707, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=722, .timer_series_len=1, .input_series_len=2, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=719}}, .input_series={(game_user_in_t){.text_addr=384, .result_action_id=728},(game_user_in_t){.text_addr=393, .result_action_id=737}}, .other_series={}}, (game_state_t){.entry_series_id=744, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=751, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=403, .result_action_id=753}}, .other_series={}}, (game_state_t){.entry_series_id=763, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=1, .other_series_len=5, .timer_series={(game_timer_t){.duration=768, .recurring=0, .result_action_id=798}}, .input_series={(game_user_in_t){.text_addr=424, .result_action_id=794}}, .other_series={(game_other_in_t){.type_id=4, .result_action_id=769},(game_other_in_t){.type_id=5, .result_action_id=775},(game_other_in_t){.type_id=6, .result_action_id=778},(game_other_in_t){.type_id=0, .result_action_id=784},(game_other_in_t){.type_id=1, .result_action_id=788}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=804}}, .input_series={(game_user_in_t){.text_addr=427, .result_action_id=800}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=427, .result_action_id=808}}, .other_series={}}, (game_state_t){.entry_series_id=813, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=823, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=555, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}};

#define SPECIAL_BADGESNEARBY0 0
#define SPECIAL_BADGESNEARBYSOME 1
#define SPECIAL_NAME_NOT_FOUND 2
#define SPECIAL_NAME_FOUND 3
#define SPECIAL_CONNECT_SUCCESS_NEW 4
#define SPECIAL_CONNECT_SUCCESS_OLD 5
#define SPECIAL_CONNECT_FAILURE 6
#define STATE_ID_FIRSTBOOT 0
#define STATE_ID_CUSTOMSTATENAMESEARCH 1
#define STATE_ID_CUSTOMSTATESTATUS 2
#define STATE_ID_FIRSTBOOTCONFUSED 3
#define STATE_ID_FIRSTBOOTHELLO 4
#define STATE_ID_FIRSTBOOTBROKEN 5
#define STATE_ID_FIRSTBOOTHELLOSPEAK 6
#define STATE_ID_FIRSTBOOTHELLOLEARN 7
#define STATE_ID_FIRSTBOOTSETSTATE 8
#define STATE_ID_SETCOUNTRY 9
#define STATE_ID_SETLANGUAGE 10
#define STATE_ID_INITIALIZELANGUAGE 11
#define STATE_ID_WELCOMETOSKIPPY 12
#define STATE_ID_WHOAREYOU 13
#define STATE_ID_STARTINGTOLEARN 14
#define STATE_ID_WEHAVENAMES 15
#define STATE_ID_LIGHTSON 16
#define STATE_ID_SKIPPYSTORY1 17
#define STATE_ID_SKIPPYSTORY2 18
#define STATE_ID_SKIPPYSTORY3 19
#define STATE_ID_WEHAVEAMISSION 20
#define STATE_ID_LOCALDISCOVERYNOTWO 21
#define STATE_ID_DOASCAN 22
#define STATE_ID_LOCALDEVICESCAN 23
#define STATE_ID_FIRSTCONTACT 24
#define STATE_ID_WHATDIDYOUFIND 25
#define STATE_ID_WHATDIDYOUFINDDETAILS 26
#define STATE_ID_CRACKINGTHEFILE1 27
#define STATE_ID_CRACKINGTHEFILE2 28
#define STATE_ID_CRACKINGTHEFILE3 29
#define STATE_ID_DOYOUHAVEALLTHEFILE 30
#define STATE_ID_CRACKINGTHEFILE4 31
#define STATE_ID_HEYSKIPPY 32
#define STATE_ID_CRACKINGTHEFILE5 33
#define STATE_ID_WHATISTHEFILE 34
#define STATE_ID_IKNOWQUEERCON 35
#define STATE_ID_DONTKNOWQUEERCON 36
#define STATE_ID_JUSTJOINED 37
#define STATE_ID_FROMTHEBEGINNING 38
#define STATE_ID_FAIRLYWELL 39
#define STATE_ID_FILELIGHTSON 40
#define STATE_ID_WHYTHEFILEISENCRYPTED 41
#define STATE_ID_CONNECTEDTONETWORK 42
#define STATE_ID_ANGRYSLEEP 43
#define STATE_ID_SLEEP 44
#define STATE_ID_DOYOUHAVESPEAKERS 45
#define STATE_ID_HOWOLDAREYOU 46
#define STATE_ID_COLORFINDER 47
#define OTHER_ACTION_CUSTOMSTATEUSERNAME 0
#define OTHER_ACTION_NAMESEARCH 1
#define OTHER_ACTION_SET_CONNECTABLE 2
#define OTHER_ACTION_CONNECT 3
#define OTHER_ACTION_STATUS_MENU 4
#define OTHER_ACTION_TURN_ON_THE_LIGHTS_TO_REPRESENT_FILE_STATE 5
#define CLOSABLE_STATES 8




////


uint8_t text_cursor = 0;
game_state_t loaded_state;
game_action_t loaded_action;
uint8_t loaded_text[25];
uint16_t state_last_special_event = GAME_NULL;
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

void load_state(game_state_t *dest, uint16_t id) {
    memcpy(dest, &all_states[id], sizeof(game_state_t));
}

void load_text(uint8_t *dest, uint16_t id) {
    if (id < main_text_len) {
        memcpy(current_text, main_text[id], 24);
    } else {
        memcpy(current_text, aux_text[id - main_text_len], 24);
    }
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
        // TODO: This is _really_ just a bigass OR, since every if statement
        //       has the same body.

        // TODO: We currently have a special firing OVER AND OVER AND OVER.
        //       This is a problem.

        // TODO: We should probably only consider these when we're LEAVING
        //       an action sequence (including an empty entry sequence). And
        //       most definitely NOT do the same one we just did.

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
                s_gd_success == 2) { // TODO
            s_gd_success = 0;
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_SUCCESS_OLD &&
                s_gd_success == 1) { // TODO
            s_gd_success = 0;
            fire_special = 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_FAILURE &&
                s_gd_failure) { // TODO
            s_gd_failure = 0;
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

    load_state(&loaded_state, state_id);

    current_state = &loaded_state;
    current_state_id = state_id;
    state_last_special_event = GAME_NULL;

    start_action_series(current_state->entry_series_id);
}

extern uint16_t gd_starting_id;

void game_begin() {
    all_animations[0] = anim_lsw;
    all_animations[1] = anim_lwf;
    all_animations[2] = anim_spinblue;
    all_animations[3] = anim_whitediscovery;

    // TODO: stored state
    game_set_state(STATE_ID_CRACKINGTHEFILE1);
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
        draw_text(
            LCD_BTM,
            // Input text is guaranteed to be in the main array.
            main_text[current_state->input_series[text_selection-1].text_addr],
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

// TODO: move
extern uint16_t gd_curr_id;
extern uint16_t gd_starting_id;

void do_action(game_action_t *action) {
    switch(action->type) {
    case GAME_ACTION_TYPE_ANIM_TEMP:
        // Set a temporary animation
        led_set_anim(&all_animations[action->detail], 0, action->duration, 0);
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
        load_text(current_text, action->detail);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_BADGNAME:
        sprintf(current_text, main_text[action->detail], badge_conf.badge_name);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_USERNAME:
        sprintf(current_text, main_text[action->detail], badge_conf.person_name);
        begin_text_action();
        break;
    case GAME_ACTION_TYPE_TEXT_CNT:
        sprintf(current_text, main_text[action->detail], badges_nearby);
        begin_text_action();
    case GAME_ACTION_TYPE_NOP:
        break; // just... do nothing.
    case GAME_ACTION_TYPE_OTHER:
        // TODO: handle
        if (action->detail == OTHER_ACTION_CUSTOMSTATEUSERNAME) {
            textentry_begin(badge_conf.person_name, 10, 1, 1);
        } else if (action->detail == OTHER_ACTION_NAMESEARCH) {
            gd_starting_id = GAME_NULL;
            gd_curr_id = GAME_NULL;
            qc15_mode = QC15_MODE_GAME_CHECKNAME;
            // IPC GET NEXT ID from ffff (any)
            while (!ipc_tx_op_buf(IPC_MSG_ID_NEXT, &gd_starting_id, 2));
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
            while (!ipc_tx_op_buf(IPC_MSG_ID_NEXT, &gd_starting_id, 2));
            lcd111_clear(LCD_BTM);
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
        if (global_flash_lockout & FLASH_LOCKOUT_READ) {
            game_action_t next_action;
            load_action(&next_action, loaded_action.next_choice_id);
            if (is_text_type(next_action.type) &&
                    next_action.detail > main_text_len) {
                break;
                // We have to break if we're locked out of reading the flash,
                //  and the next action's text lives in the aux (flash)
                //  text list.
            }
        }

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

    // Duration only counts for text:
    if (loaded_action.type != GAME_ACTION_TYPE_TEXT)
        game_curr_elapsed = loaded_action.duration;

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

    // TODO: I'm concerned that our new way of processing special stuff,
    //       and of eliminating the durations for non-text results is going
    //       to do something funky.
    // TODO: The "AB. Someone's out there" thing isn't as punchy anymore. Why?
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
}
