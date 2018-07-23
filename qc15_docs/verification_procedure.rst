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
   * If the program output indicates incorrect MCU detection, please swap
     3-pin programming connectors between the ports on the jig labeled 
     "Game CPU" and "Display CPU". The "Flash" port is not used in this 
     procedure.
  
* Ready to test!

Testing
-------

For each badge, follow the following procedure:

A. Physical
~~~~~~~~~~~

* Remove from bag. 
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
* Visually inspect both LCD displays for bad rows or bad screens. Remember 
  any problems: testing will continue, but the badge will need to be placed
  in the rework area in part B.4.

2. Automatic Power-on Self Test (POST)
**************************************

* In the event of a power-on self-test failure, lights will turn RED, and
  the bottom screen will provide an error detail. In the event of an
  ``IPC General Failure`` error, return to B.0 and repeat procedure once,
  paying particular attention to the output of the first half of the
  flashing script for errors.

3. Button/switch test
*********************

* Badge prompts for buttons to be pressed in the following sequence: up, 
  down, left, right. Follow the prompts.
* In the event of button failures or missing buttons, discontinue testing 
  and move badge to rework rack or area.
* Badge prompts for switch to be moved back and forth.
* In the event that switch movement is not detected, return to B.0
  and repeat test procedure once. If that does not cause switch movement
  to be detected, discontinue testing, note the failure, and move badge
  to diagnosis rack or area.

4. Self-test completion
***********************

**If the badge has failed ANY self-test or inspection, discontinue
testing and set it aside for diagnosis or rework at this time.** 
**Even if prompted, do NOT select the "UP" button to complete POST.**

* Press the UP button ONLY if the badge has passed all self-tests
  and visual and hardware inspection.
* All lights turn green and upper screen reports ``FLASH PROGRAM
  MODE``.
* Badge has passed verification. Continue testing next badge.
