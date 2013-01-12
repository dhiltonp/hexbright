This program is to aid in calibrating your temperature sensor.


Finding your sensor calibration
-------------------------------


Upload the program to your flashlight.

Find 0 C:
Put your flashlight in a water glass.  Surround the light with ice.  Add cold water.  Wait 20 minutes, or until the reading stabilizes.  Record the reading.

Find a second temperature:
Put your flashlight in a thermos.  Fill it with lukewarm water (104 fahrenheit or 40 celsius works well).  Measure the temperature with a medical thermometer to verify the temperature is within range.  Wait 20 minutes, and record both the water temperature and the flashlight's reading.


 
Reading the temperature
-----------------------

Read the tail cap LEDs, as follows:

left most number of digits (first color), 
slight pause,
next left most number of digits (second color),
...
right-most number of digits (first color),
2 second pause

for example: 2 green flashes, 6 red flashes, 3 green flashes = 263



Updating your calibration
-------------------------

Test your new thermometer by replacing 'get_thermal_sensor' with 'get_fahrenheit' or 'get_celsius' in temperature_calibration.

Modify libraries/hexbright/hexbright.cpp:
modify get_celsius() to report the correct real temperature.
modify OVERHEAT_TEMPERATURE with the desired sensor reading.
(OVERHEAT_TEMPERATURE = desired_temp * (2nd_temp_reading - 0C_temp_reading) / 2nd_temp + 0C_temp_reading)

50C/120F isn't a bad limit.  Do not go over 70C/160F.  I wouldn't go over 60C/140.
