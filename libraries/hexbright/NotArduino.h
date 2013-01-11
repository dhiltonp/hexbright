

/*#ifndef __AVR__
#include "NotArduino.h"
#endif
*/

#include <cstdlib>
#include <math.h>

#define INPUT 0
#define OUTPUT 255
#define LOW 0
#define HIGH 255

int __heap_start=300;
int* __brkval=&__heap_start;

// costs us 0 in Arduino
int pgm_read_byte(int i) {
  return 0;
}

// functions pinMode through analogRead cost us 850 bytes in total.  
//  Implementing these in avr-c may be ideal.
void pinMode(unsigned char pin, unsigned char io) {
  return;
}

void digitalWrite(unsigned char pin, unsigned char lh) {
  return;
}

bool digitalRead(unsigned char pin) {
  return true;
}

void analogWrite(unsigned char pin, unsigned char lh) {
  return;
}

unsigned char analogRead(unsigned char pin) {
  return 0;
}

// costs us 80 bytes
unsigned long micros() {
  return 0;
}
// costs us 24 bytes
void delayMicroseconds(int time) {
  return;
}
unsigned long millis() {
  return 0;
}


/////////////////////
#include <iostream>

class SerialClass {
 public:
  void begin(int baud) {
    return;
  }

  void println(const char * str) {
    std::cout<<str<<std::endl;
  }
  void print(const char * str) {
    std::cout<<str;
  }

  void println(long num) {
    std::cout<<num<<std::endl;
  }
  void print(long num) {
    std::cout<<num;
  }
};

SerialClass Serial;

class WireClass {
 public:

  void begin() {
    return;
  }
  void beginTransmission(int address) {
    return;
  }
  void endTransmission(bool end=true) {
    return;
  }


  bool available() {
    return true;
  }
  unsigned char read() {
    return 0;
  }

  void write(unsigned char * bytes, int length) {
    return;
  }
  void write(unsigned char byte) {
    return;
  }

  void requestFrom(int starting_address, int _bytes) {
    return;
  }
};


WireClass Wire;

