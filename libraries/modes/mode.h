#ifndef MODE_H
#define MODE_H

class mode {
 public:
  static const char EEPROM = -1;
  static void start_mode() {};
  static void continue_mode() {};
  static void stop_mode() {};
};

#endif // MODE_H
