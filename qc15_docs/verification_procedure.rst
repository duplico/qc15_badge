Testing instructions
====================

Preparation
-----------

* Download the verification program zip file, from the following network
  location: ``\\hoover\dropbox\qc15\verification_program.zip``
  
* Unzip it, and execute ``one_time_setup.bat``, contained in the subfolder
  ``verification_program\verify_mainmcu\``.
  
* From a ``cmd`` or ``PS`` prompt, navigate to the root directory of
  ``verification_program\``, and confirm that it contains two ``.hex`` files
  and ``verify_all.bat``.

* Place a badge on the programming jig and execute ``verify_all.bat``. 
  If the program output indicates incorrect MCU detection, please swap
  3-pin programming connectors between the ports on the jig labeled 
  "Game CPU" and "Display CPU". The "Flash" port is not used in this 
  procedure.
  
* Ready to test!

Testing
-------

NOTE: In the production badge code, an altered procedure similar to this may be
accessed by pressing a button while inserting badges:
      
* Holding *no buttons* during power-up will result in a fastboot, which performs
  a power-on self-test but reports only errors and does not test buttons
  or the switch,
* Holding *down* during power-up will activate the verbose automatic power-on
  self test, which performs the complete test procedures B.2 and B.3 below.
* Holding *right* during power-up activates flash programming mode, which
  disengages the MCU's pins from the external SPI flash chip, allowing it to
  be programmed by an external programmer.
* The radio performance test is specialized code running on both MCUs and is
  unfortunately not accessible in the production code.

For each badge, follow the following procedure:

A. Physical
~~~~~~~~~~~

* Remove from bag.
* Remove film from LCD displays.
* Visually inspect to confirm all components, particularly switch and buttons.
* In the event of physical switch or button damage, do not discontinue testing.
  If the damage is hard to see, notate it with an arrow sticker.
  Continue test procedure.
* Physically inspect the switch by moving it from the OFF (toward buttons) position to the ON position (away from buttons).
* If switch is very stiff, move it back and forth to loosen it 
  slightly. Move the switch back to the ON position. Continue test procedure.

B. Electronic
~~~~~~~~~~~~~

0. Program
**********

* Place badge on programming jig.
* Execute ``verify_all.bat``. The badge may require slight pressure to maintain
  good connection to the programming jig.
* In the event of flashing errors, attempt to repeat the process while 
  maintaining good connection to the jig.
* In the event of repeated errors, discontinue testing and set aside the 
  badge in the diagnosis rack or area.

1. LED/LCD test
***************

* All lights illuminate white. 
* Visually inspect all LEDs for unlit channels, and affix arrow stickers to
  indicate any LEDs that require rework. Continue procedure.
* Both LCD displays show text.
* Visually inspect both LCD displays for bad rows or bad screens. Place an
  arrow sticker on the bezel of any LCD display with problems. *Continue
  testing*, but the badge will need to be placed in the rework area at the end 
  of procedure B.4.

2. Automatic Power-on Self Test (POST)
**************************************

* In the event of a power-on self-test failure, lights will change color, and
  the bottom screen will provide an error detail. In the event of an
  ``IPC General Failure`` error, return to B.0 and repeat procedure once,
  paying particular attention to the output of the first half of the
  flashing script for errors.

3. Button/switch test
*********************

* Badge prompts for buttons to be pressed in the following sequence: up, 
  down, left, right. Follow the prompts.
* In the event of button failures or missing buttons, place arrow stickers on
  the affected button(s), and discontinue testing. Place affected badge in 
  rework rack or area.
* Badge prompts for switch to be moved back and forth.
* In the event that switch movement is not detected, return to B.0
  and repeat test procedure once. If that does not cause switch movement
  to be detected, place an arrow sticker near the switch, discontinue testing, 
  and move badge to diagnosis rack or area.
* Press the UP button to exit automated self-test.
  
4. Radio performance test
*************************

The badge now enters radio performance test mode. It will transmit a message
every 5 seconds, lighting WHITE when it does so. It will light GREEN when it
receives a radio message.

A couple of badges should be staged, possible along with a TI Launchpad with 
LEDs that report sending and receiving.

* Monitor the badge while it sends and receives. If it appears to reliably
  (>50%) interact with other badges and the test boards, it has passed the
  radio performance test.
  
* If the badge does not reliably interact with others, it has failed the radio
  performance test and will need a new radio. Place an arrow sticker on the
  badge pointing to the radio module, and press the LEFT button. The badge will
  reboot and may now be set aside for rework.
  
**If the badge has failed ANY self-test or inspection, press the LEFT button at 
this time. Discontinue testing and set it aside for diagnosis or rework.** 
***Do not follow the steps in part 5 until the badge has been repaired
and fully verified.***
  
5. Self-test completion
***********************

* At the successful conclusion of a radio performance test, press the DOWN
  button. Do this ONLY if the badge has passed all self-tests and visual and 
  hardware inspection.
* All lights turn green and upper screen reports ``FLASH PROGRAM
  MODE``.
* Badge has passed verification. Continue testing next badge.
