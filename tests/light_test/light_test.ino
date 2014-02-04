#include <hexbright.h> // include DEBUG definitions

// alter the default behavior of the hexbright library
#define SET_LIGHT_LEVEL_SIMPLE
#define DEBUG DEBUG_LIGHT

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>

hexbright hb;

#define OFF_MODE   0
#define SWEEP_MODE 1
char mode = OFF_MODE;

void setup() {
  hb.init_hardware(); 
}

void loop() {
  hb.update();

  if(hb.button_just_released()) {
    if(mode==OFF_MODE) {
      hb.set_light(0, 1000, 10000);
      mode = SWEEP_MODE;
    } else if (mode==SWEEP_MODE) {
      hb.set_light(CURRENT_LEVEL, OFF_LEVEL, 120);
      mode = OFF_MODE;
    }
  }
}

