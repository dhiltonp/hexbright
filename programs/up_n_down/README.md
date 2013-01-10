
An excellent everyday program written by [wbattestilli](https://github.com/wbattestilli/hexbright).

------------------------------------------------

Up-n-Down is intended to be a everyday, useful program.  It requires my fork (with slight modifications) to Dave Hilton's awesome hexbright library.

Short (<350ms) clicks change the mode.  Mode can only be changed from the off state. A simple click will always turn off the flashlight when it is on. The modes are as follows:

1 Click-Normal flashlight mode.  If the flashlight is pointing at the ground it will start at the dimmest setting. If it is raised 45 degrees from the ground, it will start on a medium setting. If it is horizontal or angled up, it will start on the high setting. Once on, you can hold the button for more than 350ms and change its angle with the ground to adjust the brightness.  Let go the button to lock in the current brightness. Pointing at the ground is the lowest setting and straight up in the air is the brightest setting.

2 Clicks-Blinking mode. By default the light randomly turn on the light to full brightness or completely off.  The odds start at 1:4 that the light will be turned on.  Pointing at the ground and holding the button, the odds drop to 1:2 and point up they go as high as 1:12. Shaking the light will switch to a pure on/off blinking at about 15Hz.  Holding the button and point at the ground you can slow the blink from about 60Hz to about 0.6Hz by pointing at the ceiling. Let go the button to lock in the frequency.

3 Clicks-Nightlight mode.  When sitting still, the tailcap glows red.  When picked up, the tailcap led shuts off and the light turns on to the brightest setting.  If not moved for 5 seconds it dims and the tailcap glows red again.
