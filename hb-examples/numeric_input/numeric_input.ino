#include <input_digit.h>

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>

hexbright hb;

void setup() {
  hb.init_hardware();
  hb.set_light(1, 1, NOW);
}

// single short click: accept current number as value, continue numeric input
// single long click: accept current number as value, set light at this brightness, reset value
// double click: turn off
 
unsigned int value = 0;
void loop() {
  hb.update();
  if(hb.button_just_released() && hb.button_pressed_time()<300) { // on button press, we could do it based on whatever input signal we want.
    value = get_input_digit();
  } else if (hb.button_just_released()) {
    value = get_input_digit();
    hb.set_light(CURRENT_LEVEL, value<1 ? 1 : value, 300);
    value = 0;
  } else if (hb.button_just_pressed() && hb.button_released_time()<500) {
    hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    value = 0; 
  }
  input_digit(value*10, value*10+10);
}
