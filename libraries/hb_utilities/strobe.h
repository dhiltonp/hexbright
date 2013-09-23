#ifndef STROBE_H
#define STROBE_H

#include "hexbright.h"

// turn off strobe... (aka max unsigned long)
#define STROBE_OFF -1


// Strobing features, limitations, and usage notes:
//  - not compatible with set_light
//    (turn off light with set_light(0,0,NOW) before using strobes
//     turn off strobe with set_strobe_delay(STROBE_OFF) before using set_light)
//  - the strobe will not drift over time (hooray!)
//  - strobes will occur within a 25 microsecond window of their update time or not at all
//     (flicker indicates that we missed our 25 microsecond window)
//  - strobes occur during the update function; the longer your code takes to execute, the less strobes will occur.
//  - strobes close to a multiple of 8333 will cause update() to occasionally take slightly 
//     longer to execute, eventually snap back to the start time, something like this:
//       8.33 8.33 8.33 8.7 9.4 9.4 9.4 4.83 8.33 (9.4 = 9400 microsecond delay).
//  - strobe code ignores overheating.  Under most circumstances this should be ok.
  
// delay between strobes in microseconds.
//  STROBE_OFF turns off strobing.
extern void set_strobe_delay(unsigned long delay);

// set duration to a value between 50 and 3000.
//  below 50, you will not see the light
// 75-400 work well for strobing; 
//  above 3000 you will lose some accelerometer samples.
extern void set_strobe_duration(int duration);

  
// convenience functions:
//  example usage: 
//  set_strobe_fpm(59500);
//  get_strobe_fpm(); // returns 59523
//  get_strobe_error(); // returns 703
//  you are strobing at 59523 fpm, +- 703 fpm.
// This code requires calibration/tuning
  
//  fpm accepted is from 0-65k
//  the higher the fpm, the higher the error.
extern void set_strobe_fpm(unsigned int fpm);

// returns the actual fpm you can currently expect
extern unsigned int get_strobe_fpm();

// returns the current margin of error in fpm
extern unsigned int get_strobe_error();




#define UPDATE_SPIN // use the alternative update spin associated with this .h file (found in 
//extern void update_spin();

extern unsigned long next_strobe;
extern unsigned long strobe_delay;
extern int strobe_duration;





#endif // STROBE_H
