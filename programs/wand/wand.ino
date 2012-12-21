#include <Wire.h>

// uncomment #ACCELEROMETER in hexbright.h
#include <hexbright.h>

hexbright hb(18);

void setup() {
  hb.init_hardware(); 
}

#define OFF_MODE 0
#define WAND_MODE 1
#define JAB_MODE 2

int mode = OFF_MODE;
int level = 0;

void loop() {
  hb.update();
  if(hb.button_released() && hb.button_held()<300) {
    mode = WAND_MODE; 
  } else if(hb.button_released() && hb.button_held()<700) {
    mode = JAB_MODE; 
  } else if (hb.button_held()>1000) {
    level = 0;
    mode = OFF_MODE; 
  }
  
  if(mode==JAB_MODE) {
    double jab = hb.jab_detect();
    if(jab>.5) {
//      hb.print_accelerometer(); // and dot product, cos_alpha
      level+=jab*50;
      Serial.println(jab);
      if(level<0) {
        level = 0;
      } else if (level>1000) {
        level = 1000; 
      }
      hb.set_light(CURRENT_LEVEL, level, 120);  
    }
  } else if(mode==WAND_MODE) {
    static double last_dp = 1;
    static int highest_level = 1;
    double dp = hb.get_dp();
    

    // sample rate is 120hz, so every 8 ms or so we get a new reading.
    // we will be 120/10th of the way to the new brightness the next time we come through.
    // dp is in dp, 1 = gravity

//    get brighter with vigorous movement    
//    hb.set_light(CURRENT_LEVEL, abs(hb.get_dp()-1)*500, 120);

//    track activity, when activity stops, flash at the highest activity intensity.
    if(dp-last_dp>1.1) {
      highest_level = (dp-last_dp)*350;
      highest_level = highest_level>1000 ? 1000 : highest_level;
    } else if (highest_level) {
      hb.set_light(highest_level, 0, 300); 
      highest_level = 0;
    }
    last_dp = dp;
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
