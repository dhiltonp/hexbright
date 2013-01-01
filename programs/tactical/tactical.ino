#include <hexbright.h>
#include <Wire.h>

hexbright hb;

#define HOLD_TIME 250 // milliseconds before going to strobe

#define BRIGHTNESS_COUNT 4
int brightness[BRIGHTNESS_COUNT] = {1000, 600, 300, 0};
int current_brightness = BRIGHTNESS_COUNT; // start on the last mode (off)

void setup() {
  hb.init_hardware(); 
}

void loop() {
  hb.update();
  
  if(hb.button_released() && hb.button_held()<HOLD_TIME) { 
    // if held for less than 200 ms before release, change regular modes
    current_brightness = (current_brightness+1)%BRIGHTNESS_COUNT;
    set_light();
  } else if (hb.button_held()>HOLD_TIME) {
    // held for over HOLD_TIME ms, go to strobe
    static unsigned long flash_time = millis();
    if(flash_time+70<millis()) { // flash every 70 milliseconds
      flash_time = millis(); // reset flash_time
      hb.set_light(MAX_LEVEL, 0, 20); // and pulse (going from max to min over 20 milliseconds)
      // actually, because of the refresh rate, it's more like 'go from max brightness on high
      //  to max brightness on low to off.
    }
    if(hb.button_released()) { // we have been doing strobe, but the light is now released
      // after strobe, go to previous light level:
      set_light();
      // or shutdown:
      //current_brightness = 0;
      //hb.shutdown();
    }
  }
}

void set_light() {
  if(brightness[current_brightness] == 0)
    hb.shutdown();
  else
    hb.set_light(CURRENT_LEVEL, brightness[current_brightness], 50);
}

