#include "input_digit.h"

static unsigned int read_value = 0;

unsigned int get_input_digit() {
  return read_value;
}

void input_digit(unsigned int min_digit, unsigned int max_digit) {
  unsigned int tmp2 = 999 - atan2(hexbright::vector(0)[0], hexbright::vector(0)[2])*159 - 500; // scale from 0-999, counterclockwise = higher
  tmp2 = (tmp2*(max_digit-min_digit))/1000+min_digit;
  if(tmp2 == read_value) {
    if(!printing_number()) {
      print_number(tmp2);
    }
  } else {
    reset_print_number();
    hexbright::set_led(GLED,100);
  }
  read_value = tmp2; 
}
