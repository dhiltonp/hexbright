#ifndef CLICK_SWITCHER_H
#define CLICK_SWITCHER_H

#include "hexbright.h"
#include "mode.h" 
#include "switcher.h"
#include "click_counter.h"

//typedef click_switcher switcher

template <class A, class B, class C, class D, class E>
bool switcher<A,B,C,D,E>::mode_changed() {
  byte mode = click_count();
  if(switcher<A,B,C,D,E>::get_current_mode() != mode) {
    switcher<A,B,C,D,E>::set_current_mode(mode);
    return true;
  }
  return false;
} 

#endif // CLICK_SWITCHER_H
