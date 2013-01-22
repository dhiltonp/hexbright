#include <hexbright.h>
#include <Wire.h>

hexbright hb;

void setup() {
  hb.init_hardware(); 
}

void loop() {
  char tmp[10];
  hb.update();
  if(hb.samples(0)==1) {
    sprintf(tmp, "%d", hb.samples(1));
    hb.print_vector(hb.vector(1), tmp);
  }
}
