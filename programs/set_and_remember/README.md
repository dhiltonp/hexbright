
A modification to the excellent everyday program written by [wbattestilli](https://github.com/wbattestilli/hexbright).

------------------------------------------------

Set_and_Remember
================

Set_and_Remember is intended to be an everyday, useful program.  It requires Dave Hilton's awesome hexbright library. It is basically a copy of up_n_down with a couple of changes. Instead of setting the brightness when it is first turned on, it utilizes a stored level that was set and stored previously using the held click maneuver. It also has additional code in it to solve the problem where the lock is engaged the first time it is used after having been programmed.

Basic Operation
----------------
* Short (<350ms) clicks change the mode.  Mode can only be changed from the off state. A simple click will always turn off the flashlight when it is on. The modes are as follows:

*   1 Click-Normal flashlight mode
    *   Will turn the light on the stored light level. The level is stored in EEPROM so it remember between runs.
    *   Once on, you can hold the button for more than 350ms and change its angle in relation to the ground to adjust the brightness.  Let go the button to lock in the current brightness and save it. Pointing at the ground is the lowest setting and horizontal is the brightest setting.

*   2 Clicks-Blinking mode
    *   If the flashlight is pointing at the ground it will start blinking at about 0.1Hz
    *   If it is raised 45 degrees from the ground, it will start blinking at about 1.6Hz
    *   If it is horizontal or angled up, it will start blinking at about 15Hz
    *   Holding the button and point and changing the angle of the light with the ground you can speed up or slow down the frequency of the light.

*   3 Clicks-Nightlight mode
    *   When sitting still, the tailcap glows red.  When picked up, the light turns on to the brightest setting.  
    *   If not moved for 10 seconds it dims and the tailcap glows red again.
    *   Holding the button will allow adjustment of the brightness of the light. This brightness will be saved in EEPROM and be remembered when the light is turned off.

*   4 Clicks-SOS mode
    *   Light blinks SOS repeatedly in Morse code

*   5 Clicks-Lock mode
    *   When locked, the light will not turn on.  5 clicks when locked will unlock the light.

Extras
----------------
*   Holding the button for longer than 350 ms will cause the light to blink at about 15 Hz while the button is held unless you are pointing at the ground.

*   While pointing at the ground, holding the light for 3 seconds or more will cause the button to glow green even when the button is released. 
