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

#define OFF_MODE 0
#define SET_MODE 1
#define SLEEP_MODE 2
#define WAKE_MODE 3

int mode = OFF_MODE;

#define PLACES 3
int time[] = {12, 6, 10}; // 0-11 hours, 0-50 minutes, 0-9 minutes
char place = 0;
unsigned int duration = 0; // HHMM
int wake_light_level = 0;



void loop() {
  hb.update(); 
  if(hb.button_pressed_time()>3000) {
    mode = OFF_MODE;
    hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
  }
  switch (mode) {
  case OFF_MODE:
    if(hb.button_just_released()) {
      mode = SET_MODE;
      hb.set_light(0, 150, 300);
    }
    if(!printing_number()) {
      print_charge(GLED); 
    }
    break;
  case SET_MODE:
    input_digit(duration*10, duration*10+time[place]);
    if(hb.button_just_released() && hb.button_pressed_time()<300) {
      duration = get_input_digit();
      place++;
    }
    if(place==PLACES) {
      mode = SLEEP_MODE;
      wake_light_level = hb.get_light_level(); // save the current light level for our wake level
      hb.set_light(wake_light_level/2, 0, 180000);  // turn off over 3 minutes
      // set alarm, so to speak
      setTime(0); // reset the time, for easy comparison
    }
    break;
  case SLEEP_MODE:
    
    if(duration_remaining(duration) == 0) {
      hb.set_light(1, wake_light_level, 180000);
      mode = WAKE_MODE;
    } else {
      // display our current wait time...
      if(!printing_number()) {
        // print hours, and minutes remaining
        print_number(duration_remaining(duration));
      } 
    }
    break;
  case WAKE_MODE:
    
    break;
  }
}

int duration_remaining(int duration) {
  char minutes_remaining = duration%100 - minute();
  int hours_remaining = duration/100 - hour();
  if(minutes_remaining<0) {
    hours_remaining--;
    minutes_remaining+=60;
  }
  return hours_remaining*100+minutes_remaining;
}
