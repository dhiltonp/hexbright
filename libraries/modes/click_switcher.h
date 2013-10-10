#ifndef CLICK_SWITCHER_H
#define CLICK_SWITCHER_H

#include "hexbright.h"
#include "mode.h" 
#include "switcher.h"
#include "click_counter.h"

//typedef click_switcher switcher


#ifndef SWITCHER_TIMEOUT
#define SWITCHER_TIMEOUT 1000
#endif

int max_button_released_time = 0;

template <class A, class B, class C, class D, class E, class F>
bool switcher<A,B,C,D,E,F>::mode_changed() {
  int button_released_time = hexbright::button_released_time();
  max_button_released_time = button_released_time > max_button_released_time ? button_released_time : max_button_released_time;
  
  byte mode = click_count();
    
  if(switcher<A,B,C,D,E,F>::get_current_mode() != mode && mode>0) {
	if(max_button_released_time > SWITCHER_TIMEOUT) {
		mode = 0; // off
	}
    switcher<A,B,C,D,E,F>::set_current_mode(mode);
    return true;
  }
  return false;
} 

#endif // CLICK_SWITCHER_H
