// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>

hexbright hb;

void setup() {
  hb.init_hardware();
}

int frequency[] = {1, 8, 64, 256, 1024};
int i = 0;

unsigned long this_delay = 0;
unsigned long last_delay = 0;

long s_delay = 0;
#define ON 100
void loop() {
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
    buffer = buffer;
    Serial.println(buffer); 
    s_delay = buffer-ON;
    s_delay = s_delay<0 ? 0 : s_delay;
  }
  digitalWrite(10, HIGH);
  delayMicroseconds(ON);
  digitalWrite(10, LOW);
    
  while (this_delay-last_delay<s_delay) {
    this_delay = micros();
  }
  last_delay = this_delay;
}


#define OFF_MODE 0
#define STROBE_MODE 1
int mode = OFF_MODE;
/*
void loop() {
  hb.update();
  if(hb.button_held()<500 && hb.button_released()) {
    setPwmFrequency(10,frequency[i]);
    i = (i+1)%5;
    s_delay = 500;
    mode = STROBE_MODE;
//    hb.set_light(400, 400, NOW);
  } else if (hb.button_held()>1000) {
    hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    // reset variables in case we're plugged into USB
    i = 0;
    s_delay = 0;
    mode = OFF_MODE;
  }
  if(mode==STROBE_MODE) {
    // increase/decrease generally seem to be multiplicative
//    s_delay+=hb.get_spin()*3;
    unsigned long buffer = 0;
    if(Serial.available()>0)
      do {
        char tmp = Serial.read();
        delayMicroseconds(1000);
        tmp-='0';
        buffer *= 10;
        buffer += tmp;
      } while (Serial.available()>0);
    if(buffer>0) {
      Serial.println(buffer); 
      s_delay = buffer;
    }
    hb.set_strobe_delay(s_delay);
    if(!hb.printing_number()) {
      // print frames per second
      Serial.println(1000000/s_delay/60*4);
      int tmp1 = micros();
      int tmp2 = micros();
      Serial.println(tmp1);
      Serial.println(tmp2);
      hb.print_number(1000000/(s_delay)/60*4);
    }
  }
}
*/

/*
void loop() {
  hb.update();
  if(hb.button_held()<500 && hb.button_released()) {
    setPwmFrequency(10,frequency[i]);
    i = (i+1)%5;
    hb.set_light(400, 400, NOW);
  } else if (hb.button_held()>1000) {
    hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    i = 0;
  }
}
*/

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
