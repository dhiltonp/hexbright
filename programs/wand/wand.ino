#include <Wire.h>

// uncomment #ACCELEROMETER in hexbright.h
#include <hexbright.h>

hexbright hb(10);

void setup() {
  hb.init_hardware(); 
}

#define OFF_MODE 0
#define WAND_MODE 1

int mode = OFF_MODE;

void loop() {
  hb.update();
  if(hb.button_released() && hb.button_held()<500) {
    mode = WAND_MODE; 
  } else if (hb.button_held()>500) {
    mode = OFF_MODE; 
  }
  
  if(mode==WAND_MODE) {
    // sample rate is 120hz, so every 8 ms or so we get a new reading.
    hb.read_accelerometer_vector();
    //hb.print_vector(); // and dot product
    // we will be 120/10th of the way to the new brightness the next time we come through.
    //  in order words, to get max brightness, acceleration must be maintained over time.
    // dot_product can be from 0 to 3072, 400 is about 1G);
    hb.set_light(CURRENT_LEVEL, abs(hb.dot_product()-400)/3, 120); 
  } else if (mode==OFF_MODE) {
    hb.shutdown(); 
    if(hb.get_charge_state()==CHARGED) {
      // always runs = always on (the last parameter could be any positive value)
      hb.set_led(GLED, 1); 
    } else if (hb.get_charge_state()==CHARGING && hb.get_led_state(GLED)==LED_OFF) {
      hb.set_led(GLED, 200,200);
    }
  } 
}
