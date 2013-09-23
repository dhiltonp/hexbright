#ifndef CLICK_COUNTER_H
#define CLICK_COUNTER_H


#include "hexbright.h"

#define CLICK_OFF 0
#define CLICK_ACTIVE 1
#define CLICK_WAIT 2

// Call in setup loop if you want to use the click counter
// click_time is the maximum time a click can take before the counter resets
extern void config_click_count(word click_time);

// The number of clicks <= click_time.  Will not return a count until click_time ms after the button release.
// Will return -127 unless returning a valid count.
extern char click_count();


#endif //CLICK_COUNTER_H
