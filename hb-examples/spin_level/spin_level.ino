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

#include <Wire.h>

hexbright hb;

void setup() {
  hb.init_hardware();
}

#define OFF_MODE 0
#define SPIN_LEVEL_MODE 1
int mode = OFF_MODE;

static int brightness_level = 0;

void loop() {
  hb.update();
  if(hb.button_just_released() && hb.button_pressed_time()<300) {
    brightness_level = 1;
    hb.set_light(CURRENT_LEVEL, brightness_level, NOW);
    mode = SPIN_LEVEL_MODE; 
  } else if (hb.button_pressed_time()>300) {
    hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    mode = OFF_MODE; 
  }
  
  if(mode==SPIN_LEVEL_MODE) {
    if(abs(hb.difference_from_down()-.5)<.35) { // not pointing up or down
      char spin = hb.get_spin();
      brightness_level = brightness_level + spin;
      brightness_level = brightness_level>1000 ? 1000 : brightness_level;
      brightness_level = brightness_level<1 ? 1 : brightness_level;
      hb.set_light(CURRENT_LEVEL, brightness_level, 100);
    }
  } else if (mode==OFF_MODE) {
    hb.print_charge(GLED);
  }   
}
