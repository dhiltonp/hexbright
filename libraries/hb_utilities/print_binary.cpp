#include <print_binary.h>

void print_binary(int value) {
  String s = String(value, BIN);

  while(s.length()<16) {
    s = "0"+s;
  }
  Serial.println(s);
}

void print_binary(byte value) {
  String s = String(value, BIN);

  while(s.length()<8) {
    s = "0"+s;
  }
  Serial.println(s);
}
