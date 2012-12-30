#include <Wire.h>
// uncomment '#define ACCELEROMETER' in hexbright.h
#include <hexbright.h>


hexbright hb;

void setup() {
  hb.init_hardware(); 
}

void loop() {
  hb.update();
  if(hb.button_held()) {
    // http://cache.freescale.com/files/sensors/doc/data_sheet/MMA7660FC.pdf (look at page 14)
    byte tilt = hb.read_accelerometer(ACC_REG_TILT);
    print_binary(tilt);
    if(tilt & B01000000) {
      Serial.println("Bad read (accelerometer was writing to the register)");
      return;
    } 
    if(tilt & B10000000) {
      Serial.println("Shake!");
    } 
    if(tilt & B00100000) {
      Serial.println("Tap!");
    }
    if(tilt & B00000001) {
      Serial.println("Lying on front");      
    }
    if(tilt & B00000010) {
      Serial.println("Lying on back");      
    }
    if(tilt & B00011100 == B00000100) {
      Serial.println("Left landscape");      
    }
    if(tilt & B00011100 == B00010100) {
      Serial.println("Right landscape");      
    }
    if(tilt & B00011100 == B00010100) {
      Serial.println("Down");      
    }
    if(tilt & B00011100 == B00011000) {
      Serial.println("Up");      
    }
  } else {
    // http://cache.freescale.com/files/sensors/doc/app_note/AN3461.pdf
    //hb.read_accelerometer_vector();
    hb.print_vector(hb.vector(0), "read vector"); // and dot product
  }
}



void print_binary(int value) {
  String s = String(value, BIN);

  while(s.length()<16) {
    s = "0"+s;
  }
  Serial.println(s);
}

void print_binary(byte value) {
  String s = String(value, BIN);

  while(s.length()<8) {
    s = "0"+s;
  }
  Serial.println(s);
}


/*double sqrt_sqr(int vector) {
  double result = 0;
  for(int i=0; i<3;i++) {
    result += vectors[vector][i]*vectors[vector][i];
  }
  return sqrt(result);  
}

double something() {
  return dot_product()/(sqrt_sqr(0)*sqrt_sqr(1));
}*/


