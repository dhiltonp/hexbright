#ifndef SPIN_MODE_H
#define SPIN_MODE_H

#include "mode.h"
#include "hexbright.h"

static int brightness_level = 0;
class spin_mode : public mode {
 public:
  static void start_mode() {
    hexbright::set_light(CURRENT_LEVEL, brightness_level, NOW);
  };

  static void continue_mode() {
    if(abs(hexbright::difference_from_down()-.5)<.35) { // acceleration is not along the light axis, where noise causes random fluctuations.
      char spin = hexbright::get_spin();
      brightness_level = brightness_level + spin;
      brightness_level = brightness_level>1000 ? 1000 : brightness_level;
      brightness_level = brightness_level<1 ? 1 : brightness_level;
      hexbright::set_light(CURRENT_LEVEL, brightness_level, 100);
    }
  };

  static void stop_mode() {
    hexbright::set_light(CURRENT_LEVEL, 0, NOW);
  };
};


#endif // SPIN_MODE_H
