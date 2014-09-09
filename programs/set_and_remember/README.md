
A modification to the excellent everyday program written by [wbattestilli](https://github.com/wbattestilli/hexbright).

------------------------------------------------

Set_and_Remember
================

Set_and_Remember is intended to be an everyday, useful program.  It requires Dave Hilton's awesome hexbright library. It is basically a copy of up_n_down with a several changes. Those changes are:

*   1. Instead of setting the brightness when the light is first turned on based on the level to which it is pointing, it reads a previously stored level from EEPROM. 
*   2. Added code to solve the initial lock mode problem. 
*   3. Nightlight mode glows the tailcap green and turns it off when the main light is on.
*   4. When entering or exiting locked mode, the tailcap is flashed 3 times to confirm. While in locked mode, any click besides unlock results in the tail flashing red 2 times.
*   5. The implementation for morse code signalling has been rewritten to save space. It also uses the brightness stored in mode 1 for the flashes.

Basic Operation
----------------
* Short (<350ms) clicks change the mode.  Mode can only be changed from the off state. A simple click will always turn off the flashlight when it is on. The modes are as follows:

*   1 Click-Normal flashlight mode
    *   Will turn the light on a stored light level. The level is stored in EEPROM so it is remembered between runs.
    *   Once on, you can hold the button for more than 350ms and change its angle in relation to the ground to adjust the brightness.  Let go of the button to lock in the current brightness. Pointing at the ground is the lowest setting and horizontal is the brightest setting.

*   2 Clicks-Blinking mode
    *   If the flashlight is pointing at the ground it will start blinking at about 0.1Hz
    *   If it is raised 45 degrees from the ground, it will start blinking at about 1.6Hz
    *   If it is horizontal or angled up, it will start blinking at about 15Hz
    *   Holding the button and changing the angle of the flashlight with the ground allows you to alter the flash rate as described in turning it on.

*   3 Clicks-Nightlight mode
    *   When sitting still, the tailcap glows green.  When picked up, the main light turns on to the saved nightlight setting and the tail cap turns off.  
    *   If not moved for 5 seconds the main light dims to off and the tailcap starts to glow green again.
    *   Holding the button while the main light is on will allow adjustment of the brightness in the same manner as mode 1. This nightlight brightness will be saved in EEPROM so it is remembered between runs.

*   4 Clicks-SOS mode
    *   Light blinks the morse code for SOS using the brightness level set in mode 1.

*   5 Clicks-Lock mode
    *   When locked, the light will not turn on.  Any clicks while locked will cause the tail cap to flash red 2 times. Clicking 5 times when locked will unlock the light. After locking or unlocking the tail cap leds will flash 3 times as confirmation. Red for entering locked mode and green for exiting locked mode.

Extras
----------------
*   Holding the button for longer than 350 ms will cause the light to blink at about 15 Hz while the button is held unless you are pointing at the ground.

*   While pointing at the ground, holding the light for 3 seconds or more will cause the button to glow green even when the button is released. 
