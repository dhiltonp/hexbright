
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
    *   By default the light will blink at about 15 hz.
    *   Holding the button and point at the ground you can speed up the blink from about 50Hz to about 8Hz by pointing at the ceiling. Let go the button to lock in the frequency. 
    *   By tapping the light you switch to random mode.  The light is randomly turned on the light to full brightness for 20ms.  The odds start at 1:4 that the light will be turned on.  Pointing at the ground and holding the button, the odds drop to 1:2 and point up they go as high as 1:12.


*   3 Clicks-Nightlight mode
    *   When sitting still, the tailcap glows red.  When picked up, the light turns on to the brightest setting.  If not moved for 3 seconds it dims and the tailcap glows red again.

Extras
----------------
*   Holding the button for longer than 350 ms will cause the light to blink at about 15 hz while the button is held unless you are pointing at the ground.
    *    While blinking you can tap the light to change to a solid beam.
    *    This setting is saved so the next time you hold the button it will use your last setting (beam or blink).


*    If pointing at the ground, holding the light for 3 seconds or more will cause the button to glow green even when the button is released. 
