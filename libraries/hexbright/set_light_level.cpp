#include "hexbright.h"

void set_light_level_linear(unsigned long level) {
  // sets actual light level, altering value to be perceptually linear, based on steven's area brightness (cube root)

  // LOW 255 approximately equals HIGH 48/49.  There is a color change.
  // Values < 4 do not provide any light.
  // I don't know about relative power draw.
  
  // look at linearity_test.ino for more detail on these algorithms.
  
  digitalWriteFast(DPIN_PWR, HIGH);
  if(level == 0) {
    // lowest possible power, but cpu still running (DPIN_PWR still high)
    digitalWriteFast(DPIN_DRV_MODE, LOW);
    analogWrite(DPIN_DRV_EN, 0);
  } else if(level == OFF_LEVEL) {
    // power off (DPIN_PWR LOW)
    digitalWriteFast(DPIN_PWR, LOW);
    digitalWriteFast(DPIN_DRV_MODE, LOW);
    analogWrite(DPIN_DRV_EN, 0);
  } else { 
    byte value;
    if(level<=500) {
      digitalWriteFast(DPIN_DRV_MODE, LOW);
      value = (byte)(.000000633*(level*level*level)+.000632*(level*level)+.0285*level+3.98);
    } else {
      level -= 500;
      digitalWriteFast(DPIN_DRV_MODE, HIGH);
      value = (byte)(.00000052*(level*level*level)+.000365*(level*level)+.108*level+44.8);
    }
    analogWrite(DPIN_DRV_EN, value);
  }
}


void set_light_level_simple(unsigned long level) {
  // Values < 4 do not provide any light.
  digitalWriteFast(DPIN_PWR, HIGH);
  if(level == OFF_LEVEL) {
    // power off (DPIN_PWR LOW)
    digitalWriteFast(DPIN_PWR, LOW);
    digitalWriteFast(DPIN_DRV_MODE, LOW);
    analogWrite(DPIN_DRV_EN, 0);
  } else { 
    byte value;
    if(level<=500) {
      digitalWriteFast(DPIN_DRV_MODE, LOW);
      value = level/1.96078431373; // costs about 40 bytes more than a bit-shift
    } else {
      level -= 500;
      value = level/1.96078431373;
      digitalWriteFast(DPIN_DRV_MODE, HIGH);
    }
    analogWrite(DPIN_DRV_EN, value);
  }
}
