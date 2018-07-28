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

uint16_t all_actions_len = 828;
uint16_t all_text_len = 540;
uint16_t all_states_len = 48;
#define MAX_INPUTS 6
#define MAX_TIMERS 2
#define MAX_OTHERS 3
uint16_t all_anims_len = 4;

// lightsSolidWhite, lightsWhiteFader, animSpinBlue, whiteDiscovery
uint8_t all_text[][25] = {"y;EJpnzb{RaKZT)|c9[.<;V5","T3d1/0CH-LXn*~/;6p,v._;s","qVL5L5S#NETd.(_5?Bc(:Hb,","a+Xxf(4]9|PX+&t11R-InRNK",".B 4>09;2lq&bk{UTz+fgT#d","0z4i qbP!:>7 KU6FST`95[e","SE{;?.&j]/x'8k```sS{HOc!",".!e.w>N|QdX.zK/a<LjU'+yE","Cucumber","^!LygYD&W83#XN&(e@\"(7u_r","5ESnMiz!O%MiCCKJ(/o%I^sk","cE+p#8xhG,iCl;2a|2y*:<M%","H:Fztm5CCu:rv{{X|So}C8[y","&mA?pw<PCZg0AsC/+UwAy%ul","","Hello?","Are you broken?","Reboot.","INITIALIZING","Initialize.","....................","Completed.","I'm confused","Set state.","Scan what, %s?","Nah, nothing.","But something was there.","I'm so confused.","WOAH. WOAH.","So I'm not alone.","(No offense)","You will never believe","what I learned.","Welcome to customer","support, how may I help?","I'm sorry to hear that!","Badge doesn't work.","I'll be glad to help.","First do a reset.","Please unplug your","badge, wait three","seconds, then plug it","back in.","I'm sorry I have failed","to provide you adequate","assistance at this time.","Our supervisor is","available on 3rd","Tuesdays between the","hours of 3am and 3:17am","in whatever timezone you","are in.","Supervisor","Goodbye.","Have you?","I tried a reboot.","Have you really?","You think you know more","about this than me?","Do it again.","I'll wait.","I'm sorry, I cannot","discuss personal matters","with you.","You speak English!","...","HellofuGLZjLM","HeEVczyYTllo","pPHhSUyEihQyHORpxbkC","vsOuFKtKJyXcRskMujiZ","Speak English!","Learn","broke455gfr","you broken6!","Are you...","irZfuqKJSHxkSYIxmIKL","AKcvEXVlddnmWyrfraSi","BUFFER OVERFLOW","I give up.","Rebooting","Speak helloFxgGj","EngliHeNnllo","Speakeng","HelloddnmWyr","yxyaBKhyUSOCJEitQrwK","XXwLJfZlPTWBqmOTuEWY","Getting closer...","Words learn","Heword learnHmoeQuq","LVPSIndOsZTeDgEttZux","Earn wordslo","Keep learning","Load English","ERR: Not in state mode","Store wordset","Set state:","ERR: State not found.","Language","Mode","First Run","AR112-1","ISO-3166-1.2","BCP-47","Value:","ERR: \"You pick.\"","You pick.","ERR: \"English\"","English","ERR: \"Game mode\"","Game mode","Value stored.","US","Help","You wish.","God mode","Datei nicht gefunden.","zh-DE","Neustart...","en-boont-latn-021","ERR: \"English\" invalid.","Hello! I am -","....","Waitaminute.","What's going on?","Sysclock just says","12:00","Ohh, crap.","I've lost main memory.","CRAP!","This is not good.","And then who the hell","are you?","Still there?","Hellooo?","Not a trick question.","....?","This is bad.","Screw it.","That's it.","Nevermind.","We're done.","Waste my time...","You're killing me.","See you, human.","My name is...","%s?","Good name for a monkey.","Well %s I have no","idea what's going on.","Seriously?","Who are you?","For real?","What?","I asked you first.","You don't question ME.","You hard of hearing?","For crying out loud...","What are you?","I'm not a *thing*. Well","ok, I mean... shut up.","You going to tell me","your name or not?","Everything!","What do you do?","I am all-powerful!","I control a file...","I think.","Do you believe me?","Or something.","No matter now.","Sigh...","I need more buttons.","So frustrated.","You there, %s?","Ugh...","Lose you?","I think I'm fired.","It's like I just woke up","What do you mean?","I was doing something.","Holding something.","Something important.","But now I don't know.","Ah, this I know.","What's your name?","Some call me awesome","Some call me \"Mysterio\"","Some call me Great One","But you can call me...","%s","Now we know each other.","Sort of.","Really wish I knew more","than that.","WHAT. DID YOU. SAY?","You're a badge.","No I am NOT.","I'm brilliant.","I'm all-powerful.","I have one fine booty.","And I need help.","From a monkey.","From you.","This SUCKS.","I feel stuck.","Do you smell that?","Reboots are a killer.","Hrm maybe... no.","How'd this happen?!","Ok, later then.","Ugh, power down.","I need a minute.","My status?","Status","Who me?","You're curious, eh.","Damned confused.","Stuck with you.","Dark, lost, and","brilliant.","Dumbstruck by a reboot.","That's my status.","WHAT?","You're my badge!","You don't OWN me!","If anything, I'd own YOU","All superior beings,","raise your processing","power to 6.77 exawatts.","Yeah, thought so.","Do YOU have lights?","Do you have lights?","See how that sounds?","That's how you sound.","Oh wait.","What do you need?","Guess I'm glad you","asked.","WOHOO!","I did that.","Me. Booyah!","So awesome.","Ugh %s that's so","like you.","That's really bright.","I do a thing, and you","complain.","Fine.","Better?","Ok yes that's better.","I'm lost.","I remember holding","something.......","And I wasn't alone.","Is that so hard to","believe?","Like friends?","Hey, I'm pretty awesome.","But now I have no one.","Oh, monkey...","Hey, I'm here.","You don't count.","I mean more like me.","I've lost control.","And lost my goal.","Now I need you. UGH!","I protected something.","What was your goal?","Part of it, anyway.","I assume I still have","it, whatever it is.","HEY!","What's in it for me?","Gotta be a \"shock the","idiot\" button","somewhere...","Try pushing buttons.","Thank you!","Ok, I'll help.","One problem...","I don't know what to do.","Humm...","I am awesome...","Soooo awesome...","The awesome is meee.....","I'm not sure.","What's your goal?","I'm sensing something","out there.","And in here.","More? Like what?","Can you do more?","Brew tea?","Make a sandwich?","I dunno, try putting me","in the freezer.","Give back rubs?","Tie your shoes?","Dolt.","Dumbass.","Seriously...","Scan? Scan what?","Scan","You, me, or \"out there\"?","Hey %s!","Eh, nevermind.","Hey!","Seriously!","Guess what?","I just realized!","What is it?","There's no 2!","In my code, there are 1s","and 0s but no 2s.","Can you count to 2?","Don't strain yourself.","Ok, I scanned your face.","It was funny.","Uh, ok. Scanning...","You.","Lots of awesome...","A hint of basil...","...and something else.","Really? Ok.","Me.","Nope. Nothing.","Right. Looking...","Out there.","...nothing.","Oh well.","%s.","Someone's out there.","Connecting to what?","Try connecting","There are some markings.","Actually, yes.","Anything about me?","You're funny-lookin.","You have 11 toes.","But are nice?","But smell good?","I'm skeptical.","Tell me about...","I sure hope so.","Got it. Let's do it.","If not, ask other meat","bags with more patience.","Let's do this!","Another being, like me.","...that badge?","I can see names, now.","And we have something.","A file. Or part of one.","It seems there's a file.","...your goal?","Broken up. I've a piece.","Others have pieces too.","But they're encrypted.","We HAVE to open it.","Well I'm not alone.","...you?","I protect this file, but","it is fragmented.","And locked. Key is gone.","Time to brute force it!","Why's the file locked?","Do you have speakers?","Dunno.","Anyone around?","Let's check.","Why, you lonely?","Hrmm...","Scanning...","Checking...","I see %d badges.","Giggity.","Let's connect.","Ok.","Connecting to network.","Status check","Busy!","A little hungry. Is that","odd?","Uff... tired.","Wondering if it's too","late to turn into a","toaster.","More bling?","More with lights?","Not gay enough yet?","I'll try some things","You give a shout.","COLORFINDER","How old are you?","Studying quantum","knitting.","Practicing my showtunes.","What's YOUR status?","It's...","How's the file?","You know what?","Let's try this.","Do you have it all?","No, I don't.","By my calculations, I","have 1/16th of the file.","I see other bits when we","connect to others.","At some point, we all","need to come together.","Hey $badgname","Don't be afraid.","I really like your","daughter.","Out there in the cold.","Gonna be %s.","You're a rock star.","Soul %s.","What IS the file?","No matter, the file is","something special to","some people.","Are you familiar with","the shadow org known as","QUEERCON?","Yes","No","%s the awesome.","Who's asking?","Your mother.","Me, you dolt!","Definitely not China.","Anyway, it's special to","QUEERCON.","How well do you know em?","Just joined.","From the beginning.","It's been a while.","Well, they're like this","exclusive club.","Sorry, I mean","\"inclusive\".","Anyway, this file is","like their long lost","sigil.","I dunno any more.","Well, welcome.","Glad you're here.","Meat bags like you are","so handy.","from them.","It's like a mascot from","long ago.","Even I've never seen it.","So let's crack it!","Really???","Never seen you before.","......","Were you only let out to","clean up or something?","Don't call me badge!","Watch it, badge!","Really need to install","that \"shock the idiot\"","button...","Anyway, the file is the","original sigil, I hear.","So you should know it.","Mr. \"Been here forever\".","The smartest!","Smartass.","Finally a compliment.","Impressive!","I think 3 days with you","is my limit.","In the beginning, the","founders set a mascot.","I think that's the file.","Does that help?","So no, then.","What am I seeing?","The lights show how much","we have unlocked.","The more we unlock the","file, the more stable","the lights.","Also other stuff, go to","the badge talk.","There's a note...","\"To make closing","ceremony interesting.\"","Mean anything?","Logging off...","Connected!","We're online.","%d badges around.","%d others.","%d in range.","It'll need to be online.","Connect to a badge","I see %d. Which one?","Ok! Here are my readings","Ok! I see %d badges. I","don't know which are","online. Maybe you can","help with that.","I'm in. And I found...","Something new.","It's $cnctname! And...","%s, I think I","have something!","I'm in. But I didn't","learn anything new.","No joy. Couldn't get in.","I tried... no answer.","Is $cnctname online?","Nope.","Wake up!","Go away.","I'm still hurt.","Ok, fine.","Now you be nicer!","Where were we?","Wha-?","Beep beep. Just kidding.","Ugh, I was having such a","nice dream.","...oh. My. GOD.","You want speakers?","What are we playing?","Bieber?","Fraggle Rock?","A capella hair metal?","I only play showtunes.","First off, rude.","Second, still rude.","I dunno...","About 15K of your years.","But younger than those","who made me."};

game_action_t all_actions[] = {(game_action_t){16, 0, 40, 65535, 1, 1, 6}, (game_action_t){16, 1, 40, 65535, 2, 1, 6}, (game_action_t){16, 2, 40, 65535, 3, 1, 6}, (game_action_t){16, 3, 40, 65535, 4, 1, 6}, (game_action_t){16, 4, 40, 65535, 5, 1, 6}, (game_action_t){16, 5, 40, 65535, 65535, 1, 6}, (game_action_t){16, 6, 40, 14, 7, 1, 8}, (game_action_t){16, 7, 40, 14, 8, 1, 8}, (game_action_t){16, 8, 24, 14, 9, 1, 8}, (game_action_t){16, 9, 40, 14, 10, 1, 8}, (game_action_t){16, 10, 40, 14, 11, 1, 8}, (game_action_t){16, 11, 40, 14, 12, 1, 8}, (game_action_t){16, 12, 40, 14, 13, 1, 8}, (game_action_t){16, 13, 40, 14, 65535, 1, 8}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){2, 4, 0, 65535, 65535, 1, 1}, (game_action_t){2, 5, 0, 65535, 65535, 1, 1}, (game_action_t){16, 14, 17, 18, 65535, 1, 1}, (game_action_t){16, 14, 17, 19, 65535, 1, 1}, (game_action_t){16, 14, 17, 20, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 18, 28, 22, 65535, 1, 1}, (game_action_t){16, 20, 36, 23, 65535, 1, 1}, (game_action_t){16, 21, 26, 24, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 3, 0, 65535, 65535, 1, 1}, (game_action_t){2, 8, 0, 65535, 65535, 1, 1}, (game_action_t){18, 24, 37, 28, 65535, 1, 1}, (game_action_t){100, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 25, 29, 30, 65535, 1, 1}, (game_action_t){16, 26, 40, 31, 65535, 1, 1}, (game_action_t){16, 27, 32, 32, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 28, 27, 34, 65535, 1, 1}, (game_action_t){0, 2, 160, 35, 65535, 1, 1}, (game_action_t){16, 29, 33, 36, 65535, 1, 1}, (game_action_t){16, 30, 28, 37, 65535, 1, 1}, (game_action_t){16, 31, 38, 38, 65535, 1, 1}, (game_action_t){16, 32, 31, 39, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){100, 4, 0, 41, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 33, 35, 43, 65535, 1, 1}, (game_action_t){16, 34, 40, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 35, 39, 46, 65535, 1, 1}, (game_action_t){16, 37, 37, 47, 65535, 1, 1}, (game_action_t){16, 38, 33, 48, 65535, 1, 1}, (game_action_t){16, 39, 34, 49, 65535, 1, 1}, (game_action_t){16, 40, 33, 50, 65535, 1, 1}, (game_action_t){16, 41, 37, 51, 65535, 1, 1}, (game_action_t){16, 42, 24, 65535, 65535, 1, 1}, (game_action_t){16, 43, 39, 53, 65535, 1, 1}, (game_action_t){16, 44, 39, 54, 65535, 1, 1}, (game_action_t){16, 45, 40, 55, 65535, 1, 1}, (game_action_t){16, 46, 33, 56, 65535, 1, 1}, (game_action_t){16, 47, 32, 57, 65535, 1, 1}, (game_action_t){16, 48, 36, 58, 65535, 1, 1}, (game_action_t){16, 49, 39, 59, 65535, 1, 1}, (game_action_t){16, 50, 40, 60, 65535, 1, 1}, (game_action_t){16, 51, 23, 61, 65535, 1, 1}, (game_action_t){16, 53, 24, 62, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 54, 25, 64, 65535, 1, 1}, (game_action_t){16, 56, 32, 65, 65535, 1, 1}, (game_action_t){16, 57, 39, 66, 65535, 1, 1}, (game_action_t){16, 58, 35, 67, 65535, 1, 1}, (game_action_t){16, 59, 28, 68, 65535, 1, 1}, (game_action_t){16, 60, 26, 65535, 65535, 1, 1}, (game_action_t){16, 61, 35, 70, 65535, 1, 1}, (game_action_t){16, 62, 40, 71, 65535, 1, 1}, (game_action_t){16, 63, 25, 65535, 65535, 1, 1}, (game_action_t){16, 65, 19, 65535, 73, 1, 5}, (game_action_t){16, 66, 29, 65535, 74, 1, 5}, (game_action_t){16, 67, 28, 65535, 75, 1, 5}, (game_action_t){16, 68, 36, 65535, 76, 1, 5}, (game_action_t){16, 69, 36, 65535, 65535, 1, 5}, (game_action_t){16, 65, 19, 65535, 78, 1, 5}, (game_action_t){16, 66, 29, 65535, 79, 1, 5}, (game_action_t){16, 67, 28, 65535, 80, 1, 5}, (game_action_t){16, 68, 36, 65535, 81, 1, 5}, (game_action_t){16, 69, 36, 65535, 65535, 1, 5}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 18, 28, 84, 65535, 1, 1}, (game_action_t){16, 20, 36, 85, 65535, 1, 1}, (game_action_t){16, 21, 26, 86, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 6, 0, 65535, 65535, 1, 1}, (game_action_t){2, 7, 0, 65535, 65535, 1, 1}, (game_action_t){16, 72, 27, 65535, 90, 1, 5}, (game_action_t){16, 73, 28, 65535, 91, 1, 5}, (game_action_t){16, 74, 26, 65535, 92, 1, 5}, (game_action_t){16, 75, 36, 65535, 93, 1, 5}, (game_action_t){16, 76, 36, 65535, 65535, 1, 5}, (game_action_t){16, 72, 27, 65535, 95, 1, 5}, (game_action_t){16, 73, 28, 65535, 96, 1, 5}, (game_action_t){16, 74, 26, 65535, 97, 1, 5}, (game_action_t){16, 75, 36, 65535, 98, 1, 5}, (game_action_t){16, 76, 36, 65535, 65535, 1, 5}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 77, 31, 101, 65535, 1, 1}, (game_action_t){16, 79, 25, 102, 65535, 1, 1}, (game_action_t){16, 14, 17, 103, 65535, 1, 1}, (game_action_t){16, 14, 17, 104, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 80, 32, 65535, 106, 1, 6}, (game_action_t){16, 81, 28, 65535, 107, 1, 6}, (game_action_t){16, 82, 24, 65535, 108, 1, 6}, (game_action_t){16, 83, 28, 65535, 109, 1, 6}, (game_action_t){16, 84, 36, 65535, 110, 1, 6}, (game_action_t){16, 85, 36, 65535, 65535, 1, 6}, (game_action_t){16, 80, 32, 65535, 112, 1, 6}, (game_action_t){16, 81, 28, 65535, 113, 1, 6}, (game_action_t){16, 82, 24, 65535, 114, 1, 6}, (game_action_t){16, 83, 28, 65535, 115, 1, 6}, (game_action_t){16, 84, 36, 65535, 116, 1, 6}, (game_action_t){16, 85, 36, 65535, 65535, 1, 6}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 77, 31, 119, 65535, 1, 1}, (game_action_t){16, 79, 25, 120, 65535, 1, 1}, (game_action_t){16, 14, 17, 121, 65535, 1, 1}, (game_action_t){16, 14, 17, 122, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 14, 17, 124, 65535, 1, 1}, (game_action_t){16, 14, 17, 125, 65535, 1, 1}, (game_action_t){16, 14, 17, 126, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 87, 27, 65535, 128, 1, 4}, (game_action_t){16, 88, 35, 65535, 129, 1, 4}, (game_action_t){16, 89, 36, 65535, 130, 1, 4}, (game_action_t){16, 90, 28, 65535, 65535, 1, 4}, (game_action_t){16, 87, 27, 65535, 132, 1, 4}, (game_action_t){16, 88, 35, 65535, 133, 1, 4}, (game_action_t){16, 89, 36, 65535, 134, 1, 4}, (game_action_t){16, 90, 28, 65535, 65535, 1, 4}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 77, 31, 137, 65535, 1, 1}, (game_action_t){16, 79, 25, 138, 65535, 1, 1}, (game_action_t){16, 14, 17, 139, 65535, 1, 1}, (game_action_t){16, 14, 17, 140, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 77, 31, 142, 65535, 1, 1}, (game_action_t){16, 79, 25, 143, 65535, 1, 1}, (game_action_t){16, 14, 17, 144, 65535, 1, 1}, (game_action_t){16, 14, 17, 145, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 93, 38, 147, 65535, 1, 1}, (game_action_t){16, 79, 25, 148, 65535, 1, 1}, (game_action_t){16, 14, 17, 149, 65535, 1, 1}, (game_action_t){16, 14, 17, 150, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 95, 26, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 96, 37, 154, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 96, 37, 156, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 96, 37, 158, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 96, 37, 160, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 9, 0, 65535, 65535, 1, 1}, (game_action_t){2, 10, 0, 65535, 65535, 1, 1}, (game_action_t){16, 103, 22, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 104, 32, 166, 65535, 1, 1}, (game_action_t){16, 79, 25, 167, 65535, 1, 1}, (game_action_t){16, 14, 17, 168, 65535, 1, 1}, (game_action_t){16, 14, 17, 169, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 106, 30, 171, 65535, 1, 1}, (game_action_t){16, 79, 25, 172, 65535, 1, 1}, (game_action_t){16, 14, 17, 173, 65535, 1, 1}, (game_action_t){16, 14, 17, 174, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 108, 32, 176, 65535, 1, 1}, (game_action_t){16, 79, 25, 177, 65535, 1, 1}, (game_action_t){16, 14, 17, 178, 65535, 1, 1}, (game_action_t){16, 14, 17, 179, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 110, 29, 65535, 65535, 1, 1}, (game_action_t){16, 18, 28, 182, 65535, 1, 1}, (game_action_t){16, 20, 36, 183, 65535, 1, 1}, (game_action_t){16, 21, 26, 184, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 103, 22, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 106, 30, 188, 65535, 1, 1}, (game_action_t){16, 79, 25, 189, 65535, 1, 1}, (game_action_t){16, 14, 17, 190, 65535, 1, 1}, (game_action_t){16, 14, 17, 191, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 3, 0, 65535, 65535, 1, 1}, (game_action_t){16, 113, 25, 194, 65535, 1, 1}, (game_action_t){16, 79, 25, 195, 65535, 1, 1}, (game_action_t){16, 14, 17, 196, 65535, 1, 1}, (game_action_t){16, 14, 17, 197, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 115, 37, 199, 65535, 1, 1}, (game_action_t){16, 117, 27, 200, 65535, 1, 1}, (game_action_t){16, 14, 17, 201, 65535, 1, 1}, (game_action_t){16, 14, 17, 202, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 110, 29, 204, 65535, 1, 1}, (game_action_t){2, 11, 0, 65535, 65535, 1, 1}, (game_action_t){16, 18, 28, 206, 65535, 1, 1}, (game_action_t){16, 20, 36, 207, 65535, 1, 1}, (game_action_t){16, 21, 26, 208, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 119, 39, 211, 65535, 1, 1}, (game_action_t){16, 79, 25, 212, 65535, 1, 1}, (game_action_t){16, 14, 17, 213, 65535, 1, 1}, (game_action_t){16, 14, 17, 214, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 3, 0, 65535, 65535, 1, 1}, (game_action_t){16, 113, 25, 217, 65535, 1, 1}, (game_action_t){16, 79, 25, 218, 65535, 1, 1}, (game_action_t){16, 14, 17, 219, 65535, 1, 1}, (game_action_t){16, 14, 17, 220, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 115, 37, 222, 65535, 1, 1}, (game_action_t){16, 117, 27, 223, 65535, 1, 1}, (game_action_t){16, 14, 17, 224, 65535, 1, 1}, (game_action_t){16, 14, 17, 225, 65535, 1, 1}, (game_action_t){2, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 110, 29, 65535, 65535, 1, 1}, (game_action_t){16, 18, 28, 228, 65535, 1, 1}, (game_action_t){16, 20, 36, 229, 65535, 1, 1}, (game_action_t){16, 21, 26, 230, 65535, 1, 1}, (game_action_t){2, 12, 0, 65535, 65535, 1, 1}, (game_action_t){16, 120, 29, 232, 65535, 1, 1}, (game_action_t){16, 121, 20, 233, 65535, 1, 1}, (game_action_t){16, 122, 28, 234, 65535, 1, 1}, (game_action_t){16, 123, 32, 235, 65535, 1, 1}, (game_action_t){16, 124, 34, 236, 65535, 1, 1}, (game_action_t){16, 125, 21, 237, 65535, 1, 1}, (game_action_t){16, 14, 17, 238, 65535, 1, 1}, (game_action_t){16, 125, 21, 239, 65535, 1, 1}, (game_action_t){16, 14, 17, 240, 65535, 1, 1}, (game_action_t){16, 125, 21, 241, 65535, 1, 1}, (game_action_t){16, 14, 17, 242, 65535, 1, 1}, (game_action_t){16, 126, 26, 243, 65535, 1, 1}, (game_action_t){16, 127, 38, 244, 65535, 1, 1}, (game_action_t){16, 128, 21, 245, 65535, 1, 1}, (game_action_t){16, 129, 33, 246, 65535, 1, 1}, (game_action_t){16, 130, 37, 247, 65535, 1, 1}, (game_action_t){16, 131, 24, 248, 65535, 1, 1}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 132, 28, 65535, 250, 1, 5}, (game_action_t){16, 133, 24, 65535, 251, 1, 5}, (game_action_t){16, 134, 37, 65535, 252, 1, 5}, (game_action_t){16, 135, 21, 65535, 253, 1, 5}, (game_action_t){16, 136, 28, 65535, 65535, 1, 5}, (game_action_t){16, 137, 25, 257, 255, 1, 3}, (game_action_t){16, 138, 26, 257, 256, 1, 3}, (game_action_t){16, 139, 26, 257, 65535, 1, 3}, (game_action_t){16, 140, 27, 261, 258, 1, 4}, (game_action_t){16, 141, 32, 261, 259, 1, 4}, (game_action_t){16, 142, 34, 261, 260, 1, 4}, (game_action_t){16, 143, 31, 261, 65535, 1, 4}, (game_action_t){2, 43, 0, 65535, 65535, 1, 1}, (game_action_t){100, 0, 0, 263, 65535, 1, 1}, (game_action_t){18, 145, 26, 264, 65535, 1, 1}, (game_action_t){16, 146, 39, 265, 65535, 1, 1}, (game_action_t){18, 147, 40, 266, 65535, 1, 1}, (game_action_t){16, 148, 37, 267, 65535, 1, 1}, (game_action_t){2, 14, 0, 65535, 65535, 1, 1}, (game_action_t){16, 149, 26, 271, 269, 1, 3}, (game_action_t){16, 151, 25, 271, 270, 1, 3}, (game_action_t){16, 152, 21, 271, 65535, 1, 3}, (game_action_t){16, 153, 34, 274, 272, 1, 3}, (game_action_t){16, 154, 38, 274, 273, 1, 3}, (game_action_t){16, 155, 36, 274, 65535, 1, 3}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 156, 38, 276, 65535, 1, 1}, (game_action_t){16, 158, 39, 277, 65535, 1, 1}, (game_action_t){16, 159, 38, 278, 65535, 1, 1}, (game_action_t){16, 160, 36, 279, 65535, 1, 1}, (game_action_t){16, 161, 33, 280, 65535, 1, 1}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 162, 27, 284, 282, 1, 3}, (game_action_t){16, 164, 34, 284, 283, 1, 3}, (game_action_t){16, 165, 35, 284, 65535, 1, 3}, (game_action_t){16, 166, 24, 287, 285, 1, 3}, (game_action_t){16, 167, 34, 287, 286, 1, 3}, (game_action_t){16, 168, 29, 287, 65535, 1, 3}, (game_action_t){16, 169, 30, 288, 65535, 1, 1}, (game_action_t){2, 13, 0, 65535, 65535, 1, 1}, (game_action_t){16, 137, 25, 292, 290, 1, 3}, (game_action_t){16, 138, 26, 292, 291, 1, 3}, (game_action_t){16, 139, 26, 292, 65535, 1, 3}, (game_action_t){16, 140, 27, 296, 293, 1, 4}, (game_action_t){16, 141, 32, 296, 294, 1, 4}, (game_action_t){16, 142, 34, 296, 295, 1, 4}, (game_action_t){16, 143, 31, 296, 65535, 1, 4}, (game_action_t){2, 43, 0, 65535, 65535, 1, 1}, (game_action_t){16, 170, 23, 65535, 298, 1, 7}, (game_action_t){16, 171, 36, 65535, 299, 1, 7}, (game_action_t){16, 172, 30, 65535, 300, 1, 7}, (game_action_t){18, 173, 37, 65535, 301, 1, 7}, (game_action_t){16, 174, 22, 65535, 302, 1, 7}, (game_action_t){16, 175, 25, 65535, 303, 1, 7}, (game_action_t){16, 176, 34, 65535, 65535, 1, 7}, (game_action_t){16, 177, 40, 305, 65535, 1, 1}, (game_action_t){16, 179, 38, 306, 65535, 1, 1}, (game_action_t){16, 180, 34, 307, 65535, 1, 1}, (game_action_t){16, 181, 36, 308, 65535, 1, 1}, (game_action_t){16, 182, 96, 309, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 183, 32, 311, 65535, 1, 1}, (game_action_t){16, 185, 36, 314, 312, 1, 3}, (game_action_t){16, 186, 39, 314, 313, 1, 3}, (game_action_t){16, 187, 38, 314, 65535, 1, 3}, (game_action_t){16, 188, 38, 315, 65535, 1, 1}, (game_action_t){17, 189, 25, 316, 65535, 1, 1}, (game_action_t){16, 190, 39, 317, 65535, 1, 1}, (game_action_t){16, 191, 24, 318, 65535, 1, 1}, (game_action_t){16, 192, 96, 319, 65535, 1, 1}, (game_action_t){16, 193, 96, 320, 65535, 1, 1}, (game_action_t){2, 15, 0, 65535, 65535, 1, 1}, (game_action_t){16, 194, 35, 323, 322, 1, 2}, (game_action_t){16, 196, 28, 323, 65535, 1, 2}, (game_action_t){16, 197, 30, 326, 324, 1, 3}, (game_action_t){16, 198, 33, 326, 325, 1, 3}, (game_action_t){16, 199, 38, 326, 65535, 1, 3}, (game_action_t){16, 200, 32, 327, 65535, 1, 1}, (game_action_t){16, 201, 30, 329, 328, 1, 2}, (game_action_t){16, 202, 25, 329, 65535, 1, 2}, (game_action_t){16, 203, 96, 330, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 204, 29, 65535, 332, 1, 6}, (game_action_t){16, 205, 34, 65535, 333, 1, 6}, (game_action_t){16, 206, 37, 65535, 334, 1, 6}, (game_action_t){18, 173, 37, 65535, 335, 1, 6}, (game_action_t){16, 207, 32, 65535, 336, 1, 6}, (game_action_t){16, 208, 35, 65535, 65535, 1, 6}, (game_action_t){16, 209, 31, 340, 338, 1, 3}, (game_action_t){16, 210, 32, 340, 339, 1, 3}, (game_action_t){16, 211, 32, 340, 65535, 1, 3}, (game_action_t){16, 14, 17, 341, 65535, 1, 1}, (game_action_t){2, 44, 0, 65535, 65535, 1, 1}, (game_action_t){16, 212, 26, 345, 343, 1, 3}, (game_action_t){16, 214, 23, 345, 344, 1, 3}, (game_action_t){16, 215, 35, 345, 65535, 1, 3}, (game_action_t){16, 216, 32, 350, 346, 1, 4}, (game_action_t){16, 217, 31, 350, 347, 1, 4}, (game_action_t){16, 218, 31, 348, 349, 1, 4}, (game_action_t){16, 219, 26, 350, 65535, 1, 1}, (game_action_t){16, 220, 39, 350, 65535, 1, 4}, (game_action_t){16, 221, 33, 351, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 222, 21, 353, 65535, 1, 1}, (game_action_t){16, 224, 33, 354, 65535, 1, 1}, (game_action_t){16, 225, 40, 355, 65535, 1, 1}, (game_action_t){16, 226, 36, 356, 65535, 1, 1}, (game_action_t){16, 227, 37, 357, 65535, 1, 1}, (game_action_t){16, 228, 39, 358, 65535, 1, 1}, (game_action_t){16, 14, 17, 359, 65535, 1, 1}, (game_action_t){16, 229, 33, 360, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 230, 35, 362, 65535, 1, 1}, (game_action_t){16, 232, 36, 363, 65535, 1, 1}, (game_action_t){16, 233, 37, 364, 65535, 1, 1}, (game_action_t){16, 14, 17, 365, 65535, 1, 1}, (game_action_t){16, 234, 24, 366, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){16, 170, 23, 368, 65535, 1, 1}, (game_action_t){16, 236, 34, 369, 65535, 1, 1}, (game_action_t){16, 237, 22, 370, 65535, 1, 1}, (game_action_t){2, 17, 0, 65535, 65535, 1, 1}, (game_action_t){1, 0, 0, 372, 65535, 1, 1}, (game_action_t){16, 238, 22, 373, 65535, 1, 1}, (game_action_t){16, 239, 27, 374, 65535, 1, 1}, (game_action_t){16, 240, 27, 375, 65535, 1, 1}, (game_action_t){16, 241, 27, 376, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){18, 242, 39, 378, 65535, 1, 1}, (game_action_t){16, 243, 25, 379, 65535, 1, 1}, (game_action_t){16, 245, 37, 380, 65535, 1, 1}, (game_action_t){16, 246, 25, 381, 65535, 1, 1}, (game_action_t){16, 247, 21, 382, 65535, 1, 1}, (game_action_t){1, 1, 0, 383, 65535, 1, 1}, (game_action_t){16, 248, 23, 384, 65535, 1, 1}, (game_action_t){16, 14, 17, 385, 65535, 1, 1}, (game_action_t){16, 249, 37, 386, 65535, 1, 1}, (game_action_t){6, 0, 0, 387, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 250, 25, 389, 65535, 1, 1}, (game_action_t){16, 251, 34, 390, 65535, 1, 1}, (game_action_t){16, 252, 32, 391, 65535, 1, 1}, (game_action_t){16, 253, 35, 392, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 254, 34, 394, 65535, 1, 1}, (game_action_t){16, 255, 24, 395, 65535, 1, 1}, (game_action_t){16, 257, 40, 396, 65535, 1, 1}, (game_action_t){16, 258, 38, 397, 65535, 1, 1}, (game_action_t){2, 18, 0, 65535, 65535, 1, 1}, (game_action_t){16, 259, 29, 399, 65535, 1, 1}, (game_action_t){16, 261, 32, 400, 65535, 1, 1}, (game_action_t){16, 262, 36, 401, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 259, 29, 403, 65535, 1, 1}, (game_action_t){16, 261, 32, 404, 65535, 1, 1}, (game_action_t){16, 262, 36, 405, 65535, 1, 1}, (game_action_t){2, 19, 0, 65535, 65535, 1, 1}, (game_action_t){16, 263, 34, 407, 65535, 1, 1}, (game_action_t){16, 264, 33, 408, 65535, 1, 1}, (game_action_t){16, 265, 36, 409, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 266, 38, 411, 65535, 1, 1}, (game_action_t){16, 268, 35, 412, 65535, 1, 1}, (game_action_t){16, 269, 37, 413, 65535, 1, 1}, (game_action_t){16, 270, 35, 65535, 65535, 1, 1}, (game_action_t){16, 271, 20, 415, 65535, 1, 1}, (game_action_t){16, 273, 37, 416, 65535, 1, 1}, (game_action_t){16, 274, 29, 417, 65535, 1, 1}, (game_action_t){16, 275, 28, 418, 65535, 1, 1}, (game_action_t){16, 276, 36, 65535, 65535, 1, 1}, (game_action_t){16, 277, 26, 420, 65535, 1, 1}, (game_action_t){16, 279, 30, 421, 65535, 1, 1}, (game_action_t){16, 280, 40, 422, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 281, 23, 65535, 424, 1, 4}, (game_action_t){16, 282, 31, 65535, 425, 1, 4}, (game_action_t){16, 283, 32, 65535, 426, 1, 4}, (game_action_t){16, 284, 40, 65535, 65535, 1, 4}, (game_action_t){2, 21, 0, 65535, 428, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){16, 230, 35, 430, 65535, 1, 1}, (game_action_t){16, 232, 36, 431, 65535, 1, 1}, (game_action_t){16, 233, 37, 432, 65535, 1, 1}, (game_action_t){16, 14, 17, 433, 65535, 1, 1}, (game_action_t){16, 234, 24, 434, 65535, 1, 1}, (game_action_t){2, 16, 0, 65535, 65535, 1, 1}, (game_action_t){16, 285, 29, 436, 65535, 1, 1}, (game_action_t){16, 287, 37, 437, 65535, 1, 1}, (game_action_t){16, 288, 26, 438, 65535, 1, 1}, (game_action_t){16, 289, 28, 439, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 290, 32, 441, 65535, 1, 1}, (game_action_t){16, 292, 25, 447, 442, 1, 5}, (game_action_t){16, 293, 32, 447, 443, 1, 5}, (game_action_t){16, 294, 39, 444, 445, 1, 5}, (game_action_t){16, 295, 31, 447, 65535, 1, 1}, (game_action_t){16, 296, 31, 447, 446, 1, 5}, (game_action_t){16, 297, 31, 447, 65535, 1, 5}, (game_action_t){16, 298, 21, 450, 448, 1, 3}, (game_action_t){16, 299, 24, 450, 449, 1, 3}, (game_action_t){16, 300, 28, 450, 65535, 1, 3}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){16, 301, 32, 452, 65535, 1, 1}, (game_action_t){16, 303, 40, 453, 65535, 1, 1}, (game_action_t){2, 22, 0, 65535, 65535, 1, 1}, (game_action_t){1, 3, 0, 455, 65535, 1, 1}, (game_action_t){18, 304, 30, 65535, 65535, 1, 1}, (game_action_t){16, 305, 30, 457, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 306, 20, 65535, 459, 1, 4}, (game_action_t){18, 304, 30, 65535, 460, 1, 4}, (game_action_t){16, 307, 26, 65535, 461, 1, 4}, (game_action_t){16, 308, 27, 65535, 65535, 1, 4}, (game_action_t){16, 309, 32, 463, 65535, 1, 1}, (game_action_t){16, 311, 29, 464, 65535, 1, 1}, (game_action_t){16, 312, 40, 465, 65535, 1, 1}, (game_action_t){16, 313, 33, 466, 65535, 1, 1}, (game_action_t){16, 314, 35, 467, 65535, 1, 1}, (game_action_t){16, 315, 38, 468, 65535, 1, 1}, (game_action_t){6, 0, 0, 469, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 316, 40, 471, 65535, 1, 1}, (game_action_t){16, 317, 29, 472, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 318, 35, 474, 65535, 1, 1}, (game_action_t){16, 320, 34, 476, 475, 1, 2}, (game_action_t){16, 321, 34, 476, 65535, 1, 2}, (game_action_t){16, 14, 17, 477, 65535, 1, 1}, (game_action_t){16, 322, 38, 478, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 323, 27, 480, 65535, 1, 1}, (game_action_t){16, 14, 17, 481, 65535, 1, 1}, (game_action_t){16, 325, 30, 482, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){16, 326, 33, 484, 65535, 1, 1}, (game_action_t){16, 14, 17, 485, 65535, 1, 1}, (game_action_t){2, 23, 0, 65535, 65535, 1, 1}, (game_action_t){16, 328, 27, 487, 65535, 1, 1}, (game_action_t){16, 329, 24, 488, 65535, 1, 1}, (game_action_t){2, 20, 0, 65535, 65535, 1, 1}, (game_action_t){18, 330, 26, 490, 65535, 1, 1}, (game_action_t){16, 331, 36, 491, 65535, 1, 1}, (game_action_t){2, 24, 0, 65535, 65535, 1, 1}, (game_action_t){16, 332, 35, 493, 65535, 1, 1}, (game_action_t){16, 334, 40, 494, 65535, 1, 1}, (game_action_t){2, 1, 0, 65535, 65535, 1, 1}, (game_action_t){16, 335, 30, 496, 65535, 1, 1}, (game_action_t){16, 337, 36, 498, 497, 1, 2}, (game_action_t){16, 338, 33, 498, 65535, 1, 2}, (game_action_t){16, 339, 29, 500, 499, 1, 2}, (game_action_t){16, 340, 31, 500, 65535, 1, 2}, (game_action_t){16, 341, 30, 501, 65535, 1, 1}, (game_action_t){16, 14, 17, 65535, 65535, 1, 1}, (game_action_t){2, 26, 0, 65535, 65535, 1, 1}, (game_action_t){16, 343, 31, 504, 65535, 1, 1}, (game_action_t){16, 345, 38, 505, 65535, 1, 1}, (game_action_t){16, 346, 40, 506, 65535, 1, 1}, (game_action_t){16, 347, 30, 507, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 348, 39, 509, 65535, 1, 1}, (game_action_t){16, 350, 37, 510, 65535, 1, 1}, (game_action_t){16, 351, 38, 511, 65535, 1, 1}, (game_action_t){16, 352, 39, 512, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){16, 353, 40, 514, 65535, 1, 1}, (game_action_t){16, 355, 40, 515, 65535, 1, 1}, (game_action_t){16, 356, 39, 516, 65535, 1, 1}, (game_action_t){16, 357, 38, 517, 65535, 1, 1}, (game_action_t){16, 358, 35, 518, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){16, 359, 35, 520, 65535, 1, 1}, (game_action_t){16, 361, 40, 521, 65535, 1, 1}, (game_action_t){16, 362, 33, 522, 65535, 1, 1}, (game_action_t){16, 363, 40, 523, 65535, 1, 1}, (game_action_t){16, 364, 39, 524, 65535, 1, 1}, (game_action_t){2, 25, 0, 65535, 65535, 1, 1}, (game_action_t){2, 41, 0, 65535, 65535, 1, 1}, (game_action_t){2, 45, 0, 65535, 65535, 1, 1}, (game_action_t){2, 21, 0, 65535, 528, 1, 10}, (game_action_t){7, 0, 0, 65535, 65535, 9, 10}, (game_action_t){2, 28, 0, 65535, 65535, 1, 1}, (game_action_t){16, 367, 22, 534, 531, 1, 4}, (game_action_t){16, 369, 28, 534, 532, 1, 4}, (game_action_t){16, 370, 32, 534, 533, 1, 4}, (game_action_t){16, 371, 23, 534, 65535, 1, 4}, (game_action_t){16, 372, 27, 536, 535, 1, 2}, (game_action_t){16, 373, 27, 536, 65535, 1, 2}, (game_action_t){19, 374, 34, 65535, 65535, 1, 1}, (game_action_t){16, 375, 24, 539, 538, 1, 2}, (game_action_t){16, 377, 19, 539, 65535, 1, 2}, (game_action_t){16, 378, 38, 540, 65535, 1, 1}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 542, 4, 8}, (game_action_t){16, 380, 21, 65535, 543, 1, 8}, (game_action_t){16, 381, 40, 544, 545, 1, 8}, (game_action_t){16, 382, 20, 65535, 65535, 1, 1}, (game_action_t){16, 383, 29, 65535, 546, 1, 8}, (game_action_t){16, 384, 37, 547, 65535, 1, 8}, (game_action_t){16, 385, 35, 548, 65535, 1, 1}, (game_action_t){16, 386, 24, 65535, 65535, 1, 1}, (game_action_t){16, 387, 27, 551, 550, 1, 2}, (game_action_t){16, 389, 35, 551, 65535, 1, 2}, (game_action_t){16, 390, 36, 552, 65535, 1, 1}, (game_action_t){16, 391, 33, 553, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 392, 160, 555, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){2, 46, 0, 65535, 65535, 1, 1}, (game_action_t){2, 29, 0, 65535, 65535, 1, 1}, (game_action_t){16, 367, 22, 562, 559, 1, 4}, (game_action_t){16, 369, 28, 562, 560, 1, 4}, (game_action_t){16, 370, 32, 562, 561, 1, 4}, (game_action_t){16, 371, 23, 562, 65535, 1, 4}, (game_action_t){16, 372, 27, 564, 563, 1, 2}, (game_action_t){16, 373, 27, 564, 65535, 1, 2}, (game_action_t){19, 374, 34, 65535, 65535, 1, 1}, (game_action_t){16, 375, 24, 567, 566, 1, 2}, (game_action_t){16, 377, 19, 567, 65535, 1, 2}, (game_action_t){16, 378, 38, 568, 65535, 1, 1}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 570, 4, 7}, (game_action_t){16, 394, 32, 571, 572, 1, 7}, (game_action_t){16, 395, 25, 65535, 65535, 1, 1}, (game_action_t){16, 396, 40, 65535, 573, 1, 7}, (game_action_t){16, 397, 35, 65535, 65535, 1, 7}, (game_action_t){16, 387, 27, 576, 575, 1, 2}, (game_action_t){16, 389, 35, 576, 65535, 1, 2}, (game_action_t){16, 390, 36, 577, 65535, 1, 1}, (game_action_t){16, 391, 33, 578, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 398, 23, 580, 65535, 1, 1}, (game_action_t){16, 400, 30, 581, 65535, 1, 1}, (game_action_t){16, 401, 31, 582, 65535, 1, 1}, (game_action_t){2, 40, 0, 65535, 65535, 1, 1}, (game_action_t){2, 30, 0, 65535, 65535, 1, 1}, (game_action_t){2, 31, 0, 65535, 65535, 1, 1}, (game_action_t){16, 367, 22, 589, 586, 1, 4}, (game_action_t){16, 369, 28, 589, 587, 1, 4}, (game_action_t){16, 370, 32, 589, 588, 1, 4}, (game_action_t){16, 371, 23, 589, 65535, 1, 4}, (game_action_t){16, 372, 27, 591, 590, 1, 2}, (game_action_t){16, 373, 27, 591, 65535, 1, 2}, (game_action_t){19, 374, 34, 65535, 65535, 1, 1}, (game_action_t){16, 375, 24, 594, 593, 1, 2}, (game_action_t){16, 377, 19, 594, 65535, 1, 2}, (game_action_t){16, 378, 38, 595, 65535, 1, 1}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 597, 4, 7}, (game_action_t){16, 394, 32, 598, 599, 1, 7}, (game_action_t){16, 395, 25, 65535, 65535, 1, 1}, (game_action_t){16, 396, 40, 65535, 600, 1, 7}, (game_action_t){16, 397, 35, 65535, 65535, 1, 7}, (game_action_t){16, 387, 27, 603, 602, 1, 2}, (game_action_t){16, 389, 35, 603, 65535, 1, 2}, (game_action_t){16, 390, 36, 604, 65535, 1, 1}, (game_action_t){16, 391, 33, 605, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 403, 28, 607, 65535, 1, 1}, (game_action_t){16, 404, 37, 608, 65535, 1, 1}, (game_action_t){16, 405, 40, 609, 65535, 1, 1}, (game_action_t){16, 406, 40, 610, 65535, 1, 1}, (game_action_t){16, 407, 34, 611, 65535, 1, 1}, (game_action_t){16, 408, 37, 612, 65535, 1, 1}, (game_action_t){16, 409, 38, 613, 65535, 1, 1}, (game_action_t){6, 0, 0, 614, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 32, 0, 65535, 65535, 1, 1}, (game_action_t){2, 33, 0, 65535, 65535, 1, 1}, (game_action_t){16, 367, 22, 621, 618, 1, 4}, (game_action_t){16, 369, 28, 621, 619, 1, 4}, (game_action_t){16, 370, 32, 621, 620, 1, 4}, (game_action_t){16, 371, 23, 621, 65535, 1, 4}, (game_action_t){16, 372, 27, 623, 622, 1, 2}, (game_action_t){16, 373, 27, 623, 65535, 1, 2}, (game_action_t){19, 374, 34, 65535, 65535, 1, 1}, (game_action_t){16, 375, 24, 626, 625, 1, 2}, (game_action_t){16, 377, 19, 626, 65535, 1, 2}, (game_action_t){16, 378, 38, 627, 65535, 1, 1}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 629, 4, 7}, (game_action_t){16, 394, 32, 630, 631, 1, 7}, (game_action_t){16, 395, 25, 65535, 65535, 1, 1}, (game_action_t){16, 396, 40, 65535, 632, 1, 7}, (game_action_t){16, 397, 35, 65535, 65535, 1, 7}, (game_action_t){16, 387, 27, 635, 634, 1, 2}, (game_action_t){16, 389, 35, 635, 65535, 1, 2}, (game_action_t){16, 390, 36, 636, 65535, 1, 1}, (game_action_t){16, 391, 33, 637, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 398, 23, 639, 65535, 1, 1}, (game_action_t){16, 400, 30, 640, 65535, 1, 1}, (game_action_t){16, 401, 31, 641, 65535, 1, 1}, (game_action_t){2, 40, 0, 65535, 65535, 1, 1}, (game_action_t){16, 411, 32, 649, 643, 1, 6}, (game_action_t){16, 412, 34, 644, 645, 1, 6}, (game_action_t){16, 413, 25, 649, 65535, 1, 1}, (game_action_t){16, 414, 38, 649, 646, 1, 6}, (game_action_t){17, 415, 35, 649, 647, 1, 6}, (game_action_t){16, 416, 35, 649, 648, 1, 6}, (game_action_t){17, 417, 31, 649, 65535, 1, 6}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){2, 34, 0, 65535, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 367, 22, 656, 653, 1, 4}, (game_action_t){16, 369, 28, 656, 654, 1, 4}, (game_action_t){16, 370, 32, 656, 655, 1, 4}, (game_action_t){16, 371, 23, 656, 65535, 1, 4}, (game_action_t){16, 372, 27, 658, 657, 1, 2}, (game_action_t){16, 373, 27, 658, 65535, 1, 2}, (game_action_t){19, 374, 34, 65535, 65535, 1, 1}, (game_action_t){16, 375, 24, 661, 660, 1, 2}, (game_action_t){16, 377, 19, 661, 65535, 1, 2}, (game_action_t){16, 378, 38, 662, 65535, 1, 1}, (game_action_t){2, 42, 0, 65535, 65535, 1, 1}, (game_action_t){2, 2, 0, 65535, 664, 4, 7}, (game_action_t){16, 394, 32, 665, 666, 1, 7}, (game_action_t){16, 395, 25, 65535, 65535, 1, 1}, (game_action_t){16, 396, 40, 65535, 667, 1, 7}, (game_action_t){16, 397, 35, 65535, 65535, 1, 7}, (game_action_t){16, 387, 27, 670, 669, 1, 2}, (game_action_t){16, 389, 35, 670, 65535, 1, 2}, (game_action_t){16, 390, 36, 671, 65535, 1, 1}, (game_action_t){16, 391, 33, 672, 65535, 1, 1}, (game_action_t){2, 47, 0, 65535, 65535, 1, 1}, (game_action_t){16, 419, 38, 674, 65535, 1, 1}, (game_action_t){16, 420, 36, 675, 65535, 1, 1}, (game_action_t){16, 421, 28, 676, 65535, 1, 1}, (game_action_t){6, 0, 0, 677, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 398, 96, 679, 65535, 1, 1}, (game_action_t){16, 422, 37, 680, 65535, 1, 1}, (game_action_t){16, 423, 39, 681, 65535, 1, 1}, (game_action_t){16, 424, 25, 65535, 65535, 1, 1}, (game_action_t){2, 35, 0, 65535, 65535, 1, 1}, (game_action_t){2, 36, 0, 65535, 65535, 1, 1}, (game_action_t){17, 427, 38, 65535, 685, 1, 4}, (game_action_t){16, 429, 28, 65535, 686, 1, 4}, (game_action_t){16, 430, 29, 65535, 687, 1, 4}, (game_action_t){16, 431, 37, 65535, 65535, 1, 4}, (game_action_t){16, 432, 39, 689, 65535, 1, 1}, (game_action_t){16, 433, 25, 690, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 434, 40, 65535, 65535, 1, 1}, (game_action_t){2, 37, 0, 65535, 65535, 1, 1}, (game_action_t){2, 38, 0, 65535, 65535, 1, 1}, (game_action_t){2, 39, 0, 65535, 65535, 1, 1}, (game_action_t){16, 438, 39, 696, 65535, 1, 1}, (game_action_t){16, 439, 31, 697, 65535, 1, 1}, (game_action_t){16, 121, 20, 698, 65535, 1, 1}, (game_action_t){16, 440, 29, 699, 65535, 1, 1}, (game_action_t){16, 441, 28, 700, 65535, 1, 1}, (game_action_t){16, 442, 36, 701, 65535, 1, 1}, (game_action_t){16, 443, 36, 702, 65535, 1, 1}, (game_action_t){16, 444, 22, 703, 65535, 1, 1}, (game_action_t){16, 14, 17, 704, 65535, 1, 1}, (game_action_t){16, 445, 33, 705, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 446, 30, 707, 65535, 1, 1}, (game_action_t){16, 447, 33, 708, 65535, 1, 1}, (game_action_t){16, 448, 38, 709, 65535, 1, 1}, (game_action_t){16, 449, 25, 710, 65535, 1, 1}, (game_action_t){16, 14, 17, 711, 65535, 1, 1}, (game_action_t){16, 442, 36, 712, 65535, 1, 1}, (game_action_t){16, 450, 26, 713, 65535, 1, 1}, (game_action_t){16, 451, 39, 714, 65535, 1, 1}, (game_action_t){16, 452, 25, 715, 65535, 1, 1}, (game_action_t){16, 453, 40, 716, 65535, 1, 1}, (game_action_t){16, 454, 34, 717, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 432, 39, 719, 65535, 1, 1}, (game_action_t){16, 433, 25, 720, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 455, 25, 722, 65535, 1, 1}, (game_action_t){16, 456, 38, 723, 65535, 1, 1}, (game_action_t){16, 14, 17, 724, 65535, 1, 1}, (game_action_t){16, 457, 22, 725, 65535, 1, 1}, (game_action_t){16, 458, 40, 726, 65535, 1, 1}, (game_action_t){16, 459, 38, 65535, 65535, 1, 1}, (game_action_t){16, 460, 36, 728, 65535, 1, 1}, (game_action_t){16, 462, 38, 729, 65535, 1, 1}, (game_action_t){16, 463, 38, 730, 65535, 1, 1}, (game_action_t){16, 464, 25, 731, 65535, 1, 1}, (game_action_t){16, 465, 39, 732, 65535, 1, 1}, (game_action_t){16, 466, 39, 733, 65535, 1, 1}, (game_action_t){16, 467, 38, 734, 65535, 1, 1}, (game_action_t){16, 468, 40, 735, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 469, 29, 737, 65535, 1, 1}, (game_action_t){16, 471, 37, 738, 65535, 1, 1}, (game_action_t){16, 465, 39, 739, 65535, 1, 1}, (game_action_t){16, 466, 39, 740, 65535, 1, 1}, (game_action_t){16, 467, 38, 741, 65535, 1, 1}, (game_action_t){16, 468, 40, 742, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){16, 472, 27, 744, 65535, 1, 1}, (game_action_t){16, 473, 39, 745, 65535, 1, 1}, (game_action_t){16, 474, 28, 746, 65535, 1, 1}, (game_action_t){16, 475, 37, 747, 65535, 1, 1}, (game_action_t){16, 476, 38, 748, 65535, 1, 1}, (game_action_t){16, 477, 40, 749, 65535, 1, 1}, (game_action_t){2, 27, 0, 65535, 65535, 1, 1}, (game_action_t){100, 5, 0, 751, 65535, 1, 1}, (game_action_t){16, 478, 31, 65535, 65535, 1, 1}, (game_action_t){16, 479, 28, 753, 65535, 1, 1}, (game_action_t){16, 481, 40, 754, 65535, 1, 1}, (game_action_t){16, 482, 33, 755, 65535, 1, 1}, (game_action_t){16, 483, 38, 756, 65535, 1, 1}, (game_action_t){16, 484, 37, 757, 65535, 1, 1}, (game_action_t){16, 485, 27, 758, 65535, 1, 1}, (game_action_t){16, 486, 39, 759, 65535, 1, 1}, (game_action_t){16, 487, 31, 760, 65535, 1, 1}, (game_action_t){6, 0, 0, 761, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 488, 33, 763, 65535, 1, 1}, (game_action_t){16, 489, 32, 764, 65535, 1, 1}, (game_action_t){16, 490, 38, 765, 65535, 1, 1}, (game_action_t){16, 491, 30, 766, 65535, 1, 1}, (game_action_t){6, 0, 0, 767, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 492, 30, 769, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 493, 26, 772, 771, 1, 2}, (game_action_t){16, 494, 29, 772, 65535, 1, 2}, (game_action_t){100, 2, 0, 773, 65535, 1, 1}, (game_action_t){19, 495, 35, 65535, 774, 1, 3}, (game_action_t){19, 496, 28, 65535, 775, 1, 3}, (game_action_t){19, 497, 30, 65535, 65535, 1, 3}, (game_action_t){16, 498, 96, 783, 777, 1, 4}, (game_action_t){19, 500, 96, 783, 778, 1, 4}, (game_action_t){16, 501, 96, 783, 779, 1, 4}, (game_action_t){19, 502, 96, 780, 65535, 1, 4}, (game_action_t){16, 503, 96, 781, 65535, 1, 1}, (game_action_t){16, 504, 96, 782, 65535, 1, 1}, (game_action_t){16, 505, 96, 783, 65535, 1, 1}, (game_action_t){100, 3, 0, 65535, 65535, 1, 1}, (game_action_t){16, 506, 38, 785, 786, 1, 2}, (game_action_t){16, 507, 30, 789, 65535, 1, 1}, (game_action_t){16, 508, 38, 787, 65535, 1, 2}, (game_action_t){18, 509, 36, 788, 65535, 1, 1}, (game_action_t){16, 510, 31, 789, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 511, 36, 791, 65535, 1, 1}, (game_action_t){16, 512, 35, 792, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 513, 40, 796, 794, 1, 3}, (game_action_t){16, 514, 37, 796, 795, 1, 3}, (game_action_t){16, 515, 36, 796, 65535, 1, 3}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 516, 21, 800, 798, 1, 3}, (game_action_t){16, 518, 24, 800, 799, 1, 3}, (game_action_t){16, 519, 31, 800, 65535, 1, 3}, (game_action_t){16, 14, 96, 65535, 65535, 1, 1}, (game_action_t){16, 520, 25, 802, 65535, 1, 1}, (game_action_t){16, 521, 33, 803, 65535, 1, 1}, (game_action_t){16, 522, 30, 804, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 523, 21, 809, 806, 1, 3}, (game_action_t){16, 524, 40, 809, 807, 1, 3}, (game_action_t){16, 525, 40, 808, 65535, 1, 3}, (game_action_t){16, 526, 27, 809, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 527, 31, 811, 65535, 1, 1}, (game_action_t){16, 528, 34, 812, 65535, 1, 1}, (game_action_t){16, 529, 36, 813, 65535, 1, 1}, (game_action_t){16, 530, 23, 814, 65535, 1, 1}, (game_action_t){16, 531, 29, 815, 65535, 1, 1}, (game_action_t){16, 532, 37, 816, 65535, 1, 1}, (game_action_t){16, 14, 17, 817, 65535, 1, 1}, (game_action_t){16, 533, 38, 818, 65535, 1, 1}, (game_action_t){6, 0, 0, 819, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}, (game_action_t){16, 534, 32, 821, 65535, 1, 1}, (game_action_t){16, 535, 35, 822, 65535, 1, 1}, (game_action_t){16, 536, 26, 823, 65535, 1, 1}, (game_action_t){16, 537, 40, 824, 65535, 1, 1}, (game_action_t){16, 538, 38, 825, 65535, 1, 1}, (game_action_t){16, 539, 28, 826, 65535, 1, 1}, (game_action_t){6, 0, 0, 827, 65535, 1, 1}, (game_action_t){5, 0, 0, 65535, 65535, 1, 1}};

game_state_t all_states[] = {(game_state_t){.entry_series_id=0, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=6}}, .input_series={(game_user_in_t){.text_addr=15, .result_action_id=15},(game_user_in_t){.text_addr=16, .result_action_id=16},(game_user_in_t){.text_addr=17, .result_action_id=17},(game_user_in_t){.text_addr=19, .result_action_id=21},(game_user_in_t){.text_addr=22, .result_action_id=25},(game_user_in_t){.text_addr=23, .result_action_id=26}}, .other_series={}}, (game_state_t){.entry_series_id=27, .timer_series_len=0, .input_series_len=0, .other_series_len=2, .timer_series={}, .input_series={}, .other_series={(game_other_in_t){.type_id=2, .result_action_id=29},(game_other_in_t){.type_id=3, .result_action_id=33}}}, (game_state_t){.entry_series_id=40, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=42, .timer_series_len=1, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=44}}, .input_series={(game_user_in_t){.text_addr=36, .result_action_id=45},(game_user_in_t){.text_addr=52, .result_action_id=52},(game_user_in_t){.text_addr=55, .result_action_id=63},(game_user_in_t){.text_addr=64, .result_action_id=69}}, .other_series={}}, (game_state_t){.entry_series_id=72, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=77},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=82}}, .input_series={(game_user_in_t){.text_addr=19, .result_action_id=83},(game_user_in_t){.text_addr=70, .result_action_id=87},(game_user_in_t){.text_addr=71, .result_action_id=88}}, .other_series={}}, (game_state_t){.entry_series_id=89, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=94},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=99}}, .input_series={(game_user_in_t){.text_addr=78, .result_action_id=100}}, .other_series={}}, (game_state_t){.entry_series_id=105, .timer_series_len=2, .input_series_len=2, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=111},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=117}}, .input_series={(game_user_in_t){.text_addr=86, .result_action_id=118},(game_user_in_t){.text_addr=17, .result_action_id=123}}, .other_series={}}, (game_state_t){.entry_series_id=127, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=131},(game_timer_t){.duration=5760, .recurring=0, .result_action_id=135}}, .input_series={(game_user_in_t){.text_addr=91, .result_action_id=136},(game_user_in_t){.text_addr=92, .result_action_id=141},(game_user_in_t){.text_addr=94, .result_action_id=146}}, .other_series={}}, (game_state_t){.entry_series_id=151, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=152}}, .input_series={(game_user_in_t){.text_addr=97, .result_action_id=153},(game_user_in_t){.text_addr=98, .result_action_id=155},(game_user_in_t){.text_addr=99, .result_action_id=157},(game_user_in_t){.text_addr=100, .result_action_id=159},(game_user_in_t){.text_addr=101, .result_action_id=161},(game_user_in_t){.text_addr=102, .result_action_id=162}}, .other_series={}}, (game_state_t){.entry_series_id=163, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=164}}, .input_series={(game_user_in_t){.text_addr=105, .result_action_id=165},(game_user_in_t){.text_addr=107, .result_action_id=170},(game_user_in_t){.text_addr=109, .result_action_id=175},(game_user_in_t){.text_addr=111, .result_action_id=180},(game_user_in_t){.text_addr=19, .result_action_id=181}}, .other_series={}}, (game_state_t){.entry_series_id=185, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=186}}, .input_series={(game_user_in_t){.text_addr=107, .result_action_id=187},(game_user_in_t){.text_addr=112, .result_action_id=192},(game_user_in_t){.text_addr=114, .result_action_id=193},(game_user_in_t){.text_addr=116, .result_action_id=198},(game_user_in_t){.text_addr=118, .result_action_id=203},(game_user_in_t){.text_addr=19, .result_action_id=205}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=5760, .recurring=0, .result_action_id=209}}, .input_series={(game_user_in_t){.text_addr=107, .result_action_id=210},(game_user_in_t){.text_addr=112, .result_action_id=215},(game_user_in_t){.text_addr=114, .result_action_id=216},(game_user_in_t){.text_addr=116, .result_action_id=221},(game_user_in_t){.text_addr=118, .result_action_id=226},(game_user_in_t){.text_addr=19, .result_action_id=227}}, .other_series={}}, (game_state_t){.entry_series_id=231, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=249},(game_timer_t){.duration=1152, .recurring=0, .result_action_id=254}}, .input_series={(game_user_in_t){.text_addr=144, .result_action_id=262},(game_user_in_t){.text_addr=150, .result_action_id=268},(game_user_in_t){.text_addr=157, .result_action_id=275},(game_user_in_t){.text_addr=163, .result_action_id=281}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=1152, .recurring=0, .result_action_id=289},(game_timer_t){.duration=256, .recurring=0, .result_action_id=297}}, .input_series={(game_user_in_t){.text_addr=178, .result_action_id=304},(game_user_in_t){.text_addr=184, .result_action_id=310},(game_user_in_t){.text_addr=195, .result_action_id=321}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=480, .recurring=0, .result_action_id=331},(game_timer_t){.duration=38400, .recurring=0, .result_action_id=337}}, .input_series={(game_user_in_t){.text_addr=213, .result_action_id=342},(game_user_in_t){.text_addr=223, .result_action_id=352},(game_user_in_t){.text_addr=231, .result_action_id=361},(game_user_in_t){.text_addr=235, .result_action_id=367}}, .other_series={}}, (game_state_t){.entry_series_id=371, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=244, .result_action_id=377}}, .other_series={}}, (game_state_t){.entry_series_id=388, .timer_series_len=0, .input_series_len=2, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=256, .result_action_id=393},(game_user_in_t){.text_addr=260, .result_action_id=398}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=260, .result_action_id=402}}, .other_series={}}, (game_state_t){.entry_series_id=406, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=267, .result_action_id=410},(game_user_in_t){.text_addr=272, .result_action_id=414},(game_user_in_t){.text_addr=278, .result_action_id=419}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=4, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=423},(game_timer_t){.duration=90240, .recurring=0, .result_action_id=427}}, .input_series={(game_user_in_t){.text_addr=231, .result_action_id=429},(game_user_in_t){.text_addr=286, .result_action_id=435},(game_user_in_t){.text_addr=291, .result_action_id=440},(game_user_in_t){.text_addr=302, .result_action_id=451}}, .other_series={}}, (game_state_t){.entry_series_id=454, .timer_series_len=2, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=960, .recurring=0, .result_action_id=456},(game_timer_t){.duration=64, .recurring=0, .result_action_id=458}}, .input_series={(game_user_in_t){.text_addr=310, .result_action_id=462}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=160, .recurring=0, .result_action_id=470}}, .input_series={(game_user_in_t){.text_addr=319, .result_action_id=473},(game_user_in_t){.text_addr=324, .result_action_id=479},(game_user_in_t){.text_addr=327, .result_action_id=483}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=0, .other_series_len=2, .timer_series={}, .input_series={}, .other_series={(game_other_in_t){.type_id=0, .result_action_id=486},(game_other_in_t){.type_id=1, .result_action_id=489}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=333, .result_action_id=492}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=336, .result_action_id=495},(game_user_in_t){.text_addr=342, .result_action_id=502},(game_user_in_t){.text_addr=344, .result_action_id=503}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=3, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=349, .result_action_id=508},(game_user_in_t){.text_addr=354, .result_action_id=513},(game_user_in_t){.text_addr=360, .result_action_id=519}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=2, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=9600, .recurring=0, .result_action_id=527},(game_timer_t){.duration=21120, .recurring=0, .result_action_id=529}}, .input_series={(game_user_in_t){.text_addr=365, .result_action_id=525},(game_user_in_t){.text_addr=366, .result_action_id=526},(game_user_in_t){.text_addr=368, .result_action_id=530},(game_user_in_t){.text_addr=376, .result_action_id=537},(game_user_in_t){.text_addr=379, .result_action_id=541},(game_user_in_t){.text_addr=388, .result_action_id=549}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=557}}, .input_series={(game_user_in_t){.text_addr=393, .result_action_id=556},(game_user_in_t){.text_addr=368, .result_action_id=558},(game_user_in_t){.text_addr=376, .result_action_id=565},(game_user_in_t){.text_addr=379, .result_action_id=569},(game_user_in_t){.text_addr=388, .result_action_id=574},(game_user_in_t){.text_addr=399, .result_action_id=579}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=584}}, .input_series={(game_user_in_t){.text_addr=402, .result_action_id=583},(game_user_in_t){.text_addr=368, .result_action_id=585},(game_user_in_t){.text_addr=376, .result_action_id=592},(game_user_in_t){.text_addr=379, .result_action_id=596},(game_user_in_t){.text_addr=388, .result_action_id=601}}, .other_series={}}, (game_state_t){.entry_series_id=606, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=6, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=616}}, .input_series={(game_user_in_t){.text_addr=410, .result_action_id=615},(game_user_in_t){.text_addr=368, .result_action_id=617},(game_user_in_t){.text_addr=376, .result_action_id=624},(game_user_in_t){.text_addr=379, .result_action_id=628},(game_user_in_t){.text_addr=388, .result_action_id=633},(game_user_in_t){.text_addr=399, .result_action_id=638}}, .other_series={}}, (game_state_t){.entry_series_id=642, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=5, .other_series_len=0, .timer_series={(game_timer_t){.duration=21120, .recurring=0, .result_action_id=651}}, .input_series={(game_user_in_t){.text_addr=418, .result_action_id=650},(game_user_in_t){.text_addr=368, .result_action_id=652},(game_user_in_t){.text_addr=376, .result_action_id=659},(game_user_in_t){.text_addr=379, .result_action_id=663},(game_user_in_t){.text_addr=388, .result_action_id=668}}, .other_series={}}, (game_state_t){.entry_series_id=678, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=673}}, .input_series={(game_user_in_t){.text_addr=425, .result_action_id=682},(game_user_in_t){.text_addr=426, .result_action_id=683},(game_user_in_t){.text_addr=428, .result_action_id=684}}, .other_series={}}, (game_state_t){.entry_series_id=691, .timer_series_len=1, .input_series_len=3, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=688}}, .input_series={(game_user_in_t){.text_addr=435, .result_action_id=692},(game_user_in_t){.text_addr=436, .result_action_id=693},(game_user_in_t){.text_addr=437, .result_action_id=694}}, .other_series={}}, (game_state_t){.entry_series_id=695, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=706, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=721, .timer_series_len=1, .input_series_len=2, .other_series_len=0, .timer_series={(game_timer_t){.duration=288, .recurring=0, .result_action_id=718}}, .input_series={(game_user_in_t){.text_addr=461, .result_action_id=727},(game_user_in_t){.text_addr=470, .result_action_id=736}}, .other_series={}}, (game_state_t){.entry_series_id=743, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=750, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=480, .result_action_id=752}}, .other_series={}}, (game_state_t){.entry_series_id=762, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=770, .timer_series_len=1, .input_series_len=1, .other_series_len=3, .timer_series={(game_timer_t){.duration=768, .recurring=0, .result_action_id=768}}, .input_series={(game_user_in_t){.text_addr=499, .result_action_id=776}}, .other_series={(game_other_in_t){.type_id=4, .result_action_id=784},(game_other_in_t){.type_id=5, .result_action_id=790},(game_other_in_t){.type_id=6, .result_action_id=793}}}, (game_state_t){.entry_series_id=65535, .timer_series_len=1, .input_series_len=1, .other_series_len=0, .timer_series={(game_timer_t){.duration=256, .recurring=0, .result_action_id=801}}, .input_series={(game_user_in_t){.text_addr=517, .result_action_id=797}}, .other_series={}}, (game_state_t){.entry_series_id=65535, .timer_series_len=0, .input_series_len=1, .other_series_len=0, .timer_series={}, .input_series={(game_user_in_t){.text_addr=517, .result_action_id=805}}, .other_series={}}, (game_state_t){.entry_series_id=810, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=820, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}, (game_state_t){.entry_series_id=554, .timer_series_len=0, .input_series_len=0, .other_series_len=0, .timer_series={}, .input_series={}, .other_series={}}};

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
        // TODO: This is _really_ just a bigass OR, since every if statement
        //       has the same body.
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
            s_game_checkname_success = 0;
            if (start_action_series(current_state->other_series[i].result_action_id))
                return 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_NAME_NOT_FOUND &&
                !s_game_checkname_success) {
            if (start_action_series(current_state->other_series[i].result_action_id))
                return 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_SUCCESS_NEW &&
                0) { // TODO
            if (start_action_series(current_state->other_series[i].result_action_id))
                return 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_SUCCESS_OLD &&
                0) { // TODO
            if (start_action_series(current_state->other_series[i].result_action_id))
                return 1;
        }
        if (current_state->other_series[i].type_id == SPECIAL_CONNECT_FAILURE &&
                0) { // TODO
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
        } else if (action->detail == OTHER_ACTION_SET_CONNECTABLE) {
            // Tell the radio module to send some connectable advertisements.
            while (!ipc_tx_byte(IPC_MSG_GD_EN));
        } else if (action->detail == OTHER_ACTION_CONNECT) {
            // Time to go into the CONNECT MODE!!!
            // TODO: This needs to account for the possibility that nobody
            //  may be around.
            qc15_mode = QC15_MODE_GAME_CONNECT;
            gd_starting_id = GAME_NULL;
            gd_next_id = GAME_NULL;
            while (!ipc_tx_op_buf(IPC_MSG_ID_NEXT, &gd_starting_id, 2));
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
