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
#include <print_power.h>
#include <print_number.h>

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>

#define OFF_MODE 0
#define BLINKY_MODE 1
#define CYCLE_MODE 2
#define OFF_TIME 650 // milliseconds before going off on the next normal button press
#define BRIGHTNESS_OFF 0

unsigned long short_press_time = 0;


int mode = 0;


hexbright hb;

void setup() {
  hb.init_hardware();
} 

void loop() {
  hb.update();
  static int brightness_level = 1;
  randomSeed(analogRead(0));
  int levels[] = {500,1000};

  //// Button actions to recognize, one-time actions to take as a result
  if(hb.button_just_released()) {
    if(hb.button_pressed_time()<300) { //<300 milliseconds
    
      if(short_press_time+OFF_TIME<millis() && mode!=OFF_MODE) {
        // it's been a while since our last button press, turn off
        mode = OFF_MODE;
        hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
        // in case we are under usb power, reset state
        brightness_level = 1;
      } else {
        short_press_time = millis();
        mode = CYCLE_MODE;
        brightness_level = (brightness_level+1)%2;
        hb.set_light(CURRENT_LEVEL, levels[brightness_level], 150);
      }      
      
    } else if (hb.button_pressed_time() < 1000) {
      mode = BLINKY_MODE;
    }
  }
  if(hb.button_pressed_time()>1000) { // if held for over 1000 milliseconds (whether or not it's been released), go to OFF mode
    mode = OFF_MODE;
    hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    // in case we are under usb power, reset state
    brightness_level = 1;
  }


  //// Actions over time for a given mode
  if(mode == BLINKY_MODE) { // random blink
    static int i = 0;
    if(!i) {     
     // fade from max to 0 over a random time btwn 30 and 350 milliseconds (length flash "on") 
      hb.set_light(MAX_LEVEL,0,random(30,350)); 
      // only light up every random number of times btwn 6 and 60 through
      // which should equate to 50 - 500 ms (length flash "off")
      i=random(50,500)/8.3333; 
    }
    i--;
  } else if (mode == CYCLE_MODE) { // print the current flashlight temperature
    if(!printing_number()) {
      print_number(hb.get_fahrenheit());
    }
  } else if (mode == OFF_MODE) { // charging, or turning off
    if(!printing_number()) {
      print_charge(GLED);
    }
  }
}
