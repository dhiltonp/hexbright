#include "print_number.h"

long _number = 0;
unsigned char _color = GLED;
int print_wait_time = 0;


unsigned char flip_color(unsigned char color) {
  return (color+1)%2;
}

BOOL printing_number() {
  return _number || print_wait_time;
}

void reset_print_number() {
  _number = 1;
  print_wait_time = 0;
}

void update_number() {
  if(_number>0) { // we have something to do...
#if (DEBUG==DEBUG_NUMBER)
    static int last_printed = 0;
    if(last_printed != _number) {
      last_printed = _number;
      Serial.print("number remaining (read from right to left): ");
      Serial.println(_number);
    }
#endif
    if(!print_wait_time) {
      if(_number==1) { // minimum delay between printing numbers
        print_wait_time = 2500/UPDATE_DELAY;
        _number = 0;
        return;
      } else {
        print_wait_time = 300/UPDATE_DELAY;
      }
      if(_number/10*10==_number) {
#if (DEBUG==DEBUG_NUMBER)
        Serial.println("zero");
#endif
        //        print_wait_time = 500/UPDATE_DELAY;
        hexbright::set_led(_color, 400);
      } else {
        hexbright::set_led(_color, 120);
        _number--;
      }
      if(_number && !(_number%10)) { // next digit?
        print_wait_time = 600/UPDATE_DELAY;
        _color = flip_color(_color);
        _number = _number/10;
      }
    }
  }
  
  if(print_wait_time) {
    print_wait_time--;
  }
}

void print_number(long number) {
  // reverse number (so it prints from left to right)
  BOOL negative = false;
  if(number<0) {
    number = 0-number;
    negative = true;
  }
  _color = GLED;
  _number=1; // to guarantee printing when dealing with trailing zeros (100 can't be stored as 001, use 1001)
  do {
    _number = _number * 10 + (number%10);
    number = number/10;
    _color = flip_color(_color);
  }  while(number>0);
  if(negative) {
    hexbright::set_led(flip_color(_color), 500);
    print_wait_time = 600/UPDATE_DELAY;
  }
}
