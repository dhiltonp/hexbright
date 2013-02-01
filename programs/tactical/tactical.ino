/*
Copyright (c) 2013, "David Hilton" <dhiltonp@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.
*/

#include <hexbright.h>
#include <Wire.h>

hexbright hb;

#define HOLD_TIME 250 // milliseconds before going to strobe

#define BRIGHTNESS_COUNT 4
int brightness[BRIGHTNESS_COUNT] = {1000, 600, 300, OFF_LEVEL};
int current_brightness = BRIGHTNESS_COUNT-1; // start on the last mode (off)

void setup() {
  hb.init_hardware(); 
}

void loop() {
  hb.update();
  
  if(hb.button_just_released() && hb.button_pressed_time()<HOLD_TIME) { 
    // if held for less than 200 ms before release, change regular modes
    current_brightness = (current_brightness+1)%BRIGHTNESS_COUNT;
    hb.set_light(CURRENT_LEVEL, brightness[current_brightness], 50);
  } else if (hb.button_pressed_time()>HOLD_TIME) {
    if(hb.button_pressed()) {
      // held for over HOLD_TIME ms, go to strobe
      static unsigned long flash_time = millis();
      if(flash_time+70<millis()) { // flash every 70 milliseconds
        flash_time = millis(); // reset flash_time
        hb.set_light(MAX_LEVEL, 0, 20); // and pulse (going from max to min over 20 milliseconds)
        // actually, because of the refresh rate, it's more like 'go from max brightness on high
        //  to max brightness on low to off.
      }
    } else { // we have been doing strobe, but the light is now released
      // after strobe, go to previous light level:
      hb.set_light(CURRENT_LEVEL, brightness[current_brightness], 50);
    }
  }
  hb.print_charge(GLED);
}

