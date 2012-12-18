/*
Copyright (c) 2012, "David Hilton" <dhiltonp@gmail.com>
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

#include <math.h>
#include <Wire.h>

// number of milliseconds between updates
#define LOOP_DELAY 5

#define OFF_MODE 0
#define BLINKY_MODE 1
#define CYCLE_MODE 2

int mode = 0;

//creating a hexbright object initializes the hardware
hexbright hb(LOOP_DELAY);
void setup() {} 

void loop() {
  hb.update();
// BLINKY_MODE has some issues when not connected to USB.  It detects a poweroff somehow.
// very quick releases of buttons aren't working quite right, not sure why.
  static int brightness = 0;
  if(hb.button_released()) {
    if(hb.button_held()<2) {
      // ignore, could be a bounce
    } else if(hb.button_held()<300/LOOP_DELAY) { //<300 milliseconds
      mode = CYCLE_MODE;
      brightness = (brightness + 250) % 1250; // (0/250/500/750/1000)
      brightness = brightness ? brightness : 1; // if 0, set to 1 to allow a delayed shutdown (so we can finish printing)
      hb.set_light(CURRENT_LEVEL, brightness, 150/LOOP_DELAY);
    } else if (hb.button_held() < 700/LOOP_DELAY) {
      mode = BLINKY_MODE;
    }
  }
  if(hb.button_held()>700/LOOP_DELAY) { // if held for over 700 milliseconds (whether or not it's been released), go to OFF mode
    mode = OFF_MODE;
    brightness = 0;
    hb.set_light(0,0,NOW); // go to brightness level 0 (shutdown) immediately.
  }

  if(mode == BLINKY_MODE) { // just blink - fade over 120 milliseconds, wait for 480 ms, repeat.
    static int i = 0;
    if(!i) {
      hb.set_light(MAX_LOW_LEVEL,1,120/LOOP_DELAY);
      i=600/LOOP_DELAY;
    }
    i--;
  } else if (mode == CYCLE_MODE) { // print the current flashlight temperature
    if(!hb.printing_number()) {
        hb.print_number(hb.get_fahrenheit());
      if(brightness==1) { // turn off after one more print
        brightness--;
      } else if (!brightness) {
        hb.set_light(0, 0, NOW);
      }
    }
  } else if (mode == OFF_MODE) { // can only occur when under USB power.
    if(hb.get_charge_state()==CHARGED) {
      // always runs = always on (the last parameter could be any positive value)
      hb.set_led_state(GLED, LED_ON, 1); 
    } else if (hb.get_charge_state()==CHARGING && hb.get_led_state(GLED)==LED_OFF) {
      hb.set_led_state(GLED, LED_ON, 500/LOOP_DELAY);
    } 
  }

}
