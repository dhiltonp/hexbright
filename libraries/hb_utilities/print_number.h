#ifndef PRINT_NUMBER_H
#define PRINT_NUMBER_H

#include "utilities.h"
#include "hexbright.h"

// prints a number through the rear leds
// 120 = 1 red flashes, 2 green flashes, one long red flash (0), 2 second delay.
// the largest printable value is +/-999,999,999, as the left-most digit is reserved.
// negative numbers begin with a leading long flash.
extern void print_number(long number);

// currently printing a number
extern BOOL printing_number();

// reset printing; this immediately terminates the currently printing number.
extern void reset_print_number();

// internal interface, automatically called during update
extern void update_number();

#endif // PRINT_NUMBER_H
