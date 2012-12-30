#include <Wire.h>

#include <hexbright.h>

hexbright hb;

void setup() {
  hb.init_hardware(); 
}

int i =0;
void loop() {
  i++;
  hb.update();
  if(hb.button_held()) {
//    Serial.println(hb.get_led_state(GLED));
    if(hb.get_led_state(GLED) == LED_OFF)
      hb.set_led(GLED, 250, 100); 
//      hb.set_led(GLED, 250,0); 
  } else {
    if(hb.get_led_state(RLED) == LED_OFF) {
//      Serial.println(i);
//      hb.set_led(RLED, 30,0);
//      hb.set_led(RLED, 1,1);
//      hb.set_led(RLED, 100, 1);
      hb.set_led(RLED, 500, 1000, 50);
    }
  }
}
