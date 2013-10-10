#ifndef SWITCHER_H
#define SWITCHER_H

#include "hexbright.h"


static byte current_mode;
static byte last_mode;
// unused modes/cases are removed at compile time
template<class A = mode, class B = mode, class C = mode, class D = mode, class E = mode> 
class switcher {
  public:
  static void run() {
    if(mode_changed()) {
      switch(get_last_mode()) {
      case 0:
        A::stop_mode();
        break;
      case 1:
        B::stop_mode();
        break;
      case 2:
        C::stop_mode();
        break;
      case 3:
        D::stop_mode();
        break;
      case 4:
        E::stop_mode();
        break;
      }
      switch(get_current_mode()) {
      case 0:
        A::start_mode();
        break;
      case 1:
        B::start_mode();
        break;
      case 2:
        C::start_mode();
        break;
      case 3:
        D::start_mode();
        break;
      case 4:
        E::start_mode();
        break;
      }
    } else {
      switch(get_last_mode()) {
      case 0:
        A::continue_mode();
        break;
      case 1:
        B::continue_mode();
        break;
      case 2:
        C::continue_mode();
        break;
      case 3:
        D::continue_mode();
        break;
      case 4:
        E::continue_mode();
        break;
      }
    }
  }
 private:
  static bool mode_changed();
  static byte get_current_mode() {return current_mode;} 
  static void set_current_mode(byte mode) {last_mode = current_mode; current_mode = mode;} 
  static byte get_last_mode() { return last_mode;}
};

#endif // SWITCHER_H
