// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>

hexbright hb;

void setup() {
  hb.init_hardware(); 
}

int i =0;
void loop() {
  i++;
  hb.update();
//  led_brightness_test(GLED);
  on_off_test();
//  switch_test();
}

void led_brightness_test(byte led) {
  hb.set_led(led, 50, 10, abs((i/3)%500-250));
}

void on_off_test() {
  if(hb.get_led_state(RLED) == LED_OFF)
    hb.set_led(RLED, 10,1);
}

void switch_test() {
  if(hb.button_just_released()) {
    if(hb.get_led_state(GLED) == LED_OFF)
      hb.set_led(GLED, 250, 100); 
  } else {
    if(hb.get_led_state(RLED) == LED_OFF) {
      hb.set_led(RLED, 250, 250);
    }
  }
}
