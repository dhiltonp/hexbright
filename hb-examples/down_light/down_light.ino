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

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>

hexbright hb;

void setup() {
  hb.init_hardware();
}

int smoothed_difference = 0;

#define OFF_MODE 0
#define USE_MODE 1
int mode = OFF_MODE;

void loop() {
  hb.update();

  if(hb.button_just_released() && hb.button_pressed_time()<300) {
    mode = USE_MODE; 
  } else if (hb.button_pressed_time()>300) {
    mode = OFF_MODE; 
    hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
  }
   
   
  if(USE_MODE==mode) {

    static int smoothed_difference = 0;
      if(hb.stationary()) { // low movement, use a dimmer level based on where we're pointing
      // take the average, 2 parts smoothed percent, 1 part new reading
      smoothed_difference = (smoothed_difference*5 + hb.difference_from_down()*1000)/6;
      int level = 2*(smoothed_difference);
      if(smoothed_difference<100) {
      //lots of noise, cap at a minimum.
        level = 200;
      }
      level = level>1000 ? 1000 : level;
      hb.set_light(CURRENT_LEVEL, level, 150);
    } else if (abs(hb.magnitude(hb.vector(0)))-100>50 ||
               hb.angle_change()>25) { // moderate-high movement, drop light level
       smoothed_difference=100; // reset so when we stop we build up with no jerks.
       hb.set_light(CURRENT_LEVEL, 200, 50);
    } 
  }
  print_power();
}
