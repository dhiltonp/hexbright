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

#include <print_power.h>
#include <print_number.h>
#include <input_digit.h>

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>
#include <Time.h>

hexbright hb;

void setup() {
  hb.init_hardware();
}

#define WAIT_MODE 0
#define OFF_MODE 1
#define LIGHT_SELECT_MODE 2
#define SECOND_PRESS_WAIT_MODE 3
#define SET_ACTION_MODE 4

// primary mode can be wait, light_select, or set_action
int primary_mode = WAIT_MODE;

// action mode can be wait or off
int action_mode = OFF_MODE;

#define PLACES 3
const int time[] = {12, 6, 10}; // 0-11 hours, 0-50 minutes, 0-9 minutes
char place = 0; // hours, minutes or seconds?
unsigned int action_time = 0; // set a default (accessible via short clicks) in HHMM
unsigned int previous_action_time = 0;
int action_light_level = 0;

int brightness_level = 0;


void loop() {
  hb.update();
  // primary mode selections:
  switch (primary_mode) {
  case WAIT_MODE:
    // handle action_mode
    if(action_mode == OFF_MODE && hb.light_change_remaining() == 0 && hb.get_light_level() <= 0 && !hb.button_pressed()) {
      // nothing's happening, turn off
      hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
      // or print charge state if we're plugged in
      if(!printing_number()) {
        print_charge(GLED);
      }
    } else if (action_mode == WAIT_MODE) {
      // we are waiting to do something
      if(action_time_remaining(action_time) == 0) {
        hb.set_light(CURRENT_LEVEL, action_light_level, 12000);
        action_mode=OFF_MODE;
      } else {
        // display our current wait time...
        if(!printing_number()) {
          // print hours and minutes remaining
          print_number(action_time_remaining(action_time));
        }
      }
    }
    
    // change mode?
    if(hb.button_just_released() && hb.button_pressed_time() < 300) {
      Serial.println(hb.get_light_level());
      if(hb.get_light_level() <= 0) {
        // make sure we don't turn off.  This should be modified in the api to be cleaner.
        hb.set_light(0,0,NOW); 
      }
      primary_mode = SECOND_PRESS_WAIT_MODE; 
    } else if (hb.button_pressed() && hb.button_pressed_time() > 3000) {
      primary_mode = WAIT_MODE;
      action_mode = OFF_MODE;
    } else if (hb.button_pressed() && hb.button_pressed_time() > 300) {
      brightness_level = 0;
      hb.set_light(CURRENT_LEVEL, 0, 100);
      primary_mode = WAIT_MODE;
    }
    break;
  case SECOND_PRESS_WAIT_MODE:
    if(hb.button_just_released() && hb.button_pressed_time() < 300 && hb.button_released_time() < 500) {
      primary_mode = SET_ACTION_MODE;
      action_mode = OFF_MODE;
      place = 0;
      previous_action_time = action_time;
      action_time = 0;
      action_light_level = brightness_level;
    } else if (hb.button_released_time() > 500) {
      // enter this mode after 500 ms of not being pressed
      primary_mode = LIGHT_SELECT_MODE; 
    }
    break;
  case LIGHT_SELECT_MODE:
    if(abs(hb.difference_from_down()-.5)<.35) { // not pointing up or down
      char spin = hb.get_spin();
      brightness_level = brightness_level + spin;
      brightness_level = brightness_level>1000 ? 1000 : brightness_level;
      brightness_level = brightness_level<1 ? 1 : brightness_level;
      hb.set_light(CURRENT_LEVEL, brightness_level, 100);

    }
    if(hb.button_just_released()) {
      primary_mode = WAIT_MODE; 
    }
    break;
  case SET_ACTION_MODE:
    input_digit(action_time*10, action_time*10+time[place]);
    if(hb.button_just_released() && hb.button_pressed_time()>300) {
      action_time = get_input_digit();
      place++;
    } else if (hb.button_just_released() && hb.button_pressed_time()<300) {
      // use the value from the last timer (0 on the first run)
      switch(place) {
      case 0: // hours
        action_time = (previous_action_time/100)*100;
        break;
      case 1: // greater minutes
        action_time += (previous_action_time/10)*10 % 100;
        break;
      case 2: // lesser minutes
        action_time += previous_action_time % 10;
        break;
      }
      
      place++;
    }
    if(place==PLACES) {
      primary_mode = WAIT_MODE;
      action_mode = WAIT_MODE;
      // reset the time, for easy comparison
      setTime(0);
    }
    break;
  }
  
}

int action_time_remaining(int action_time) {
  char minutes_remaining = action_time%100 - minute();
  int hours_remaining = action_time/100 - hour();
  if(minutes_remaining<0) {
    hours_remaining--;
    minutes_remaining+=60;
  }
  return hours_remaining*100+minutes_remaining;
}
