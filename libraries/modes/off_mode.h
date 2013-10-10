#ifndef OFF_MODE_H
#define OFF_MODE_H

#include "mode.h"
#include "hexbright.h"

class off_mode : public mode {
 public:
  static void start_mode() {
    hexbright::set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
  };
};


#endif // OFF_MODE_H
