Calculating Battery Life (an example):
--------------------------------------

Your battery fully charged, in prime condition has 2400 milliamp-hours.

Brightness level 150 consumes about 30 milliamps.  Let's suppose that
about 1/3rd of the time one of the rear leds is on, for whatever reason.

((9.9+3.8)/2)/3 = 2.283 milliamps consumed by the rear leds.

So, the power draw is about 32.3 mA:<br>
2400 mA-hours/32.3 mA = 74.30 hours, or about 3 days.

If you explicitly disable the accelerometer, you will save .25 milliamps:<br>
2400 mA-hours/32.05 mA = 74.88 hours, or... about 3 days.


If your battery is in perfect shape, your light should work for 3 days
straight before it runs out of power at brightness level 150.

Optimizing Power Draw
---------------------

Optimizations could increase standby time by 2x (maybe 35-40 days instead of 18 or 19 days), 
or extend battery life from 74.3 to 74.9 hours on brightness level 150. Given
the small change for a given amount of complexity/code size, I'm not planning on making this change. Further
optimization could be done by changing the electrical design. I believe the voltage regulator 
consumes 50% of the 5.4 mA in standby (see below calculations).

Formula for power loss in the voltage regulator: [(4.2 - 3.3) * .500 + (4.2*.09)](http://en.wikipedia.org/wiki/Low-dropout_regulator#Efficiency_and_Heat_Dissipation)<br>
.5 milliamps consumed by other components would cause a power loss of .83 milliamps, with 1.33 milliamps used<br>
1 milliamp consumed, loss of 1.27 milliamps, 2.27 milliamps used<br>
2 milliamps consumed, loss of 2.178, 4.178 milliamps used<br>
3 milliamps consumed, loss of 3.078, 6.078 milliamps used<br>

2.64 milliamps consumed, loss of 2.75 milliamps would be 5.4 milliamps used.
We are probably powering about 2.64 milliamps. Lowering the power used for each device will roughly double the savings listed on the data sheet.


Raw Data
--------
Recorded with a Fluke 287

brightness level, actual pwm, milliamps<br>
500, 255, 283 (~175 lumens)<br>
490, 244, 265.4<br>
480, 233, 253.5<br>
470, 222, 242.4<br>
460, 212, 230.2<br>
450, 202, 223.8<br>
440, 192, 209.6<br>
430, 183, 199.5<br>
420, 174, 190.2<br>
410, 165, 180.5<br>
400, 157, 175.1<br>
390, 148, 162.4<br>
380, 140, 153.5<br>
370, 133, 146.4<br>
360, 125, 138.2<br>
350, 118, 132.5<br>
340, 111, 123.2<br>
330, 104, 115.7<br>
320, 98, 109.5<br>
310, 92, 102.6<br>
300, 86, 97.7<br>
290, 80, 90.7<br>
280, 75, 84.6<br>
270, 70, 79.3<br>
260, 65, 73.6 (~50 lumens)<br>
250, 60, 69.5<br>
240, 55, 63.5<br>
230, 51, 58.5<br>
220, 47, 54.2<br>
210, 43, 50.3<br>
200, 40, 47.5<br>
190, 36, 42.8<br>
180, 33, 39.6<br>
170, 30, 36.4<br>
160, 27, 33.1<br>
150, 24, 30.1<br>
140, 22, 27.7<br>
130, 19, 24.6<br>
120, 17, 22.5<br>
110, 15, 20.3<br>
100, 13, 18.2<br>
90, 12, 17.1<br>
80, 10, 14.9<br>
70, 9, 13.8<br>
60, 8, 12.8<br>
50, 7, 11.5<br>
40, 6, 10.6<br>
30, 5, 9.6<br>
20, 4, 8.4<br>
10, 4, 8.4<br>
1, 4, 8.4<br>
0, 0, 5.4<br>
OFF_LEVEL, .2<br>


led, milliamps (led only, add to the power consumption of your
brightness level)<br>
GLED, 9.9<br>
RLED, 3.8<br>

So, the power consumed with the green led active, with the front led
at brightness level 1 is 9.9+8.4=18.3 mA, equivalent to just having the front
light on at 100 brightness.

The accelerometer adds about .25 milliamps and is on by default (it can be disabled by commenting out ACCELEROMETER in hexbright.h).<br>

