
An excellent everyday program written by [wbattestilli](https://github.com/wbattestilli/hexbright).

------------------------------------------------

Up-n-Down
==========

Up-n-Down is intended to be a everyday, useful program.  It requires Dave Hilton's awesome hexbright library.

Basic Operation
----------------
* Short (<350ms) clicks change the mode.  Mode can only be changed from the off state. A simple click will always turn off the flashlight when it is on. The modes are as follows:

*   1 Click-Normal flashlight mode
    *   If the flashlight is pointing at the ground it will start at the dimmest setting.
    *   If it is raised 45 degrees from the ground, it will start on a medium setting.
    *   If it is horizontal or angled up, it will start on the high setting. 
    *   Once on, you can hold the button for more than 350ms and change its angle with the ground to adjust the brightness.  Let go the button to lock in the current brightness. Pointing at the ground is the lowest setting and horizontal is the brightest setting.

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
    *   Light blinks SOS repeatededly in morse code

*   5 Clicks-Lock mode
    *   When locked, the light will not turn on.  Any clicks while locked will cause the tail cap to flash red 2 times. Clicking 5 time when locked will unlock the light. After locking or unlocking the tail cap leds will flash 3 times as confirmation. Red for entering locked mode and green for exiting locked mode.

Extras
----------------
*   Holding the button for longer than 350 ms will cause the light to blink at about 15 hz while the button is held unless you are pointing at the ground.

*   If pointing at the ground, holding the light for 3 seconds or more will cause the button to glow green even when the button is released. 
