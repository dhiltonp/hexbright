#ifndef INPUT_DIGIT_H
#define INPUT_DIGIT_H

#include "hexbright.h"
#include "print_number.h"

// reads a value between min_digit to max_digit-1, see hb-examples/numeric_input
//  Twist the light to change the value.  When the current value changes, 
//   the green LED will flash, and the number that is currently being printed 
//   will be reset.  Suppose you are at 2 and you want to go to 5.  Rotate 
//   clockwise 3 green flashes, and you'll be there.
//  The current value is printed through the rear leds.
// Get the result with get_input_digit.
extern void input_digit(unsigned int min_digit, unsigned int max_digit);

// grab the value that is currently selected (based on twist orientation)
extern unsigned int get_input_digit();

#endif // INPUT_DIGIT_H
