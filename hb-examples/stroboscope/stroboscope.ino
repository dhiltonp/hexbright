// uncomment (delete // before) '#define STROBE' in hexbright.h
#include <hexbright.h>
#include <Wire.h>

hexbright hb;

void setup() {
  hb.init_hardware(); 
}

unsigned int fpm;

#define OFF_MODE 0
#define STROBE_MODE 1
int mode = OFF_MODE;

void loop() {
  hb.update();
  
  if(mode==OFF_MODE) {
    if(hb.button_pressed_time()<200 && hb.button_just_released()) {
      fpm = 2000;
      mode = STROBE_MODE;
    }
  }
  if(mode==STROBE_MODE) {
    // turn off?
    if (hb.button_pressed_time()>200) {
      hb.set_strobe_delay(STROBE_OFF);
      hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
      mode = OFF_MODE;
      return;
    }
    
    long buffer = 0;
    if(Serial.available()>0)
      do {
        char tmp = Serial.read();
        delayMicroseconds(1000);
        tmp-='0';
        buffer *= 10;
        buffer += tmp;
      } while (Serial.available()>0);
    if(buffer>0) {
      fpm = buffer;
      hb.set_strobe_fpm(buffer);
    }
    
    char spin = hb.get_spin();
    if(abs(spin)-10>0) {  // probably not noise
      if(spin>0)
        fpm *= 1+spin/200.0;
      else
        fpm /= 1+spin/200.0; 
      //Serial.println(avg_spin);
      hb.set_strobe_fpm(fpm);
    }
    
    if(!hb.printing_number()) {
      Serial.println(hb.get_strobe_fpm());
      Serial.println(hb.get_strobe_error());
      hb.print_number(hb.get_strobe_fpm());
    }
  }
}
