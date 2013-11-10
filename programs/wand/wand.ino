#include <print_power.h>

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>

hexbright hb;

void setup() {
  hb.init_hardware(); 
}

#define OFF_MODE 0
#define WAND_MODE 1
#define JAB_MODE 2

int mode = OFF_MODE;
int level = 0;

void loop() {
  hb.update();
  if(hb.button_just_released() && hb.button_pressed_time()<500) {
    mode = WAND_MODE; 
  } else if (hb.button_pressed_time()>1000) {
    level = 0;
    mode = OFF_MODE;
    hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
  }
  
  if(mode==WAND_MODE) {
    static double last_dp = 1;
    static int highest_level = 1;
    int dp = hb.dot_product(hb.vector(0), hb.vector(1));

    // sample rate is 120hz, so every 8 ms or so we get a new reading.
    // we will be 120/10th of the way to the new brightness the next time we come through.
    // dp is in dp, 1 = gravity

//    get brighter with vigorous movement    
//    hb.set_light(CURRENT_LEVEL, abs(hb.get_dp()-1)*500, 120);

//    track activity, when activity stops, flash at the highest activity intensity.
    if(dp-last_dp>110) {
      highest_level = max((dp-last_dp)*3.5, highest_level);
      highest_level = highest_level>1000 ? 1000 : highest_level;
    } else if (highest_level) {
      hb.set_light(highest_level, 0, 300); 
      highest_level = 0;
    }
    last_dp = dp;
  } else if (mode==OFF_MODE) {
    print_power();
  } 
}
