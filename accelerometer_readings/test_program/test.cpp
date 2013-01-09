#include <iostream>

#include "../../libraries/hexbright/hexbright.h"

class hbtest : public hexbright {
public:
  static void read_accelerometer() {
    
  }
};

int main() {
  hbtest hb;
  std::cout<<hb.button_held()<<std::endl;
  return 0;
}
