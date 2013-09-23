#include "print_power.h"

void print_power() {
  print_charge(GLED);
  if (hexbright::low_voltage_state() && hexbright::get_led_state(RLED) == LED_OFF) {
    hexbright::set_led(RLED,50,1000);
  }
}

void print_charge(unsigned char led) {
  unsigned char charge_state = hexbright::get_charge_state();
  if(charge_state == CHARGING && hexbright::get_led_state(led) == LED_OFF) {
    hexbright::set_led(led, 350, 350);
  } else if (charge_state == CHARGED) {
    hexbright::set_led(led,50);
  }
}

