Some notes on hardware quirks (nearly all of this is handled internal to the library):

 * Don't read the button too frequently:
  * The rear led and button share a pin. If you're turning on the rear led, and you want to recognize button presses, you have to turn off the rear led and wait a little (50 microseconds?) for voltage to stabilize.
  * Bounce duration can be as high as 5 milliseconds, and our debouncing code only allows for one bounce (so... 6ms/2 would be the highest read frequency you would want to use).
 * When reading the current voltage (band gap reading):
  * You need to pause for 150-250 microseconds for the reading to stabilize (the workaround is built into read_adc).
 * When you run out of RAM you will get very weird errors (reboot loops, etc.). 
  * This cannot be fixed by the library, but it is a pain to debug. 
  * Use freeRam (or DEBUG_ON, which enables it automatically).
 * The charge controller can incorrectly report that it is on Battery.
  * If you're on gen1 hardware and plugged into USB, sometimes BATTERY is reported (high-Z state is between low and high). This is handled by the library.
  * If you're on gen2 hardware, plugged into USB, the light is on, and the switch is pressed, sometimes BATTERY is reported. This is NOT yet handled by the library, and also triggers a low voltage state (light won't turn on high until turned off).
 * Power draw and low voltage conditions are fairly well documented in their subdirectories.
