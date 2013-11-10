#include <print_power.h>

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>

hexbright hb;

#define OFF_MODE 0
#define CHARGE_MODE 1
#define AUTO_OFF_MODE 2

#define AUTO_OFF_WAIT 10000 // 10 seconds

unsigned int auto_off_start_time=0;
int mode=CHARGE_MODE;

void setup() {
  hb.init_hardware();
}

void loop() {
  hb.update();
  // set mode
  if(hb.button_pressed_time()>1000) { // off signal
    // turn off light
    hb.set_light(CURRENT_LEVEL,0,NOW);
    if(hb.get_charge_state()==BATTERY) { // battery power or real off?
      hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
      mode=OFF_MODE;
    } else { 
      // we're plugged in, set CHARGE_MODE (pseudo off), 
      hb.set_light(CURRENT_LEVEL, 0, NOW);
      mode=CHARGE_MODE;
    }
  }

  if(mode==CHARGE_MODE) {
    if(hb.get_charge_state()==BATTERY) { // we've changed to battery power!
      hb.set_light(CURRENT_LEVEL,150,NOW);
      mode=AUTO_OFF_MODE;
      auto_off_start_time=millis();
    } else { // flash LED because we're plugged in
      print_charge(GLED);
    }
  } else if (mode==AUTO_OFF_MODE) {
    if(millis()-auto_off_start_time > AUTO_OFF_WAIT) {
      hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    }
  }
}

