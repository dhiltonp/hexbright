#include <hexbright.h>

#include <Wire.h>

#define MS 20
hexbright hb;

#define OFF_MODE 0
#define CHARGE_MODE 1
#define AUTO_OFF_MODE 2

int auto_off_timer=0;
int mode=CHARGE_MODE;

void setup() {
  hb.init_hardware();
}

void loop() {
  hb.update();
  byte charge_state = hb.get_definite_charge_state();
  // set mode
  if(hb.button_pressed_time()>1000) { // off signal
    // turn off light
    hb.set_light(CURRENT_LEVEL,0,NOW);
    if(charge_state==BATTERY) { // battery power or real off?
      hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
      mode=OFF_MODE;
    } else { // plugged in, set CHARGE_MODE (pseudo off)
      mode=CHARGE_MODE;
    }
  }

  if(mode==CHARGE_MODE) {
    if(charge_state==BATTERY) {
      hb.set_light(CURRENT_LEVEL,150,NOW);
      mode=AUTO_OFF_MODE;
      auto_off_timer=10000/MS; // 10 seconds
    } else { // flash LED because we're plugged in
      if(hb.get_led_state(GLED)==LED_OFF)
        hb.set_led(GLED,100,350);
    }
  } else if (mode==AUTO_OFF_MODE) {
    auto_off_timer--;
    if(auto_off_timer==0) {
      hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    }
  }
}

