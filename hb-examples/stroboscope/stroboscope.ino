#include <strobe.h>
#include <print_number.h>

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>

hexbright hb;

void setup() {
  hb.init_hardware();
  hb.set_light(0,0,NOW); // don't turn off if we don't use set_light
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
      set_strobe_delay(STROBE_OFF); // if this line doesn't compile, uncomment STROBE in hexbright.h
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
      set_strobe_fpm(buffer);
    }
    
    char spin = hb.get_spin();
    if(abs(spin)-10>0) {  // probably not noise
      if(spin>0)
        fpm *= 1+spin/200.0;
      else
        fpm /= 1+spin/200.0; 
      //Serial.println(avg_spin);
      set_strobe_fpm(fpm);
    }
    
    if(!printing_number()) {
      Serial.println(get_strobe_fpm());
      Serial.println(get_strobe_error());
      print_number(get_strobe_fpm());
    }
  }
}
