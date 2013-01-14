/*
Copyright (c) 2012, "David Hilton" <dhiltonp@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.
*/

#ifdef __AVR // we're compiling for arduino
#include <Arduino.h>
#include <Wire.h>
#include <digitalWriteFast.h>
#define BOOL boolean
#else
#define BOOL bool
#endif

/// Some space-saving options
#define LED // comment out save 786 bytes if you don't use the rear LEDs
#define PRINT_NUMBER // comment out to save 626 bytes if you don't need to print numbers (but need the LEDs)
#define ACCELEROMETER //comment out to save 1500 bytes if you don't need the accelerometer
#define FLASH_CHECKSUM // comment out to save 56 bytes when in debug mode
#define FREE_RAM // comment out to save 146 bytes when in debug mode
//#define STROBE // comment out to save 260 bytes (strobe is designed for high-precision
//               //  stroboscope code, not general periodic flashing)

// The above #defines can help if you are running out of flash.  If you are having weird lockups,
//  you may be running out of ram.  See freeRam's definition for additional information.


#ifdef ACCELEROMETER
#define DPIN_ACC_INT 3

#define ACC_ADDRESS             0x4C
#define ACC_REG_XOUT            0
#define ACC_REG_YOUT            1
#define ACC_REG_ZOUT            2
#define ACC_REG_TILT            3
#define ACC_REG_INTS            6
#define ACC_REG_MODE            7

// return values for get_tilt_orientation
#define TILT_UNKNOWN 0
#define TILT_UP 1
#define TILT_DOWN 2
#define TILT_HORIZONTAL 3
#endif

// debugging related definitions
#define DEBUG 0
// Some debug modes set the light.  Your control code may reset it, causing weird flashes at startup.
#define DEBUG_OFF 0 // no extra code is compiled in
#define DEBUG_ON 1 // initialize printing
#define DEBUG_LOOP 2 // main loop
#define DEBUG_LIGHT 3 // Light control
#define DEBUG_TEMP 4  // temperature safety
#define DEBUG_BUTTON 5 // button presses - you may experience some flickering LEDs if enabled
#define DEBUG_LED 6 // rear LEDs - you may get flickering LEDs with this enabled
#define DEBUG_ACCEL 7 // accelerometer
#define DEBUG_NUMBER 8 // number printing utility
#define DEBUG_CHARGE 9 // charge state



#if (DEBUG==DEBUG_TEMP)
#define OVERHEAT_TEMPERATURE 265 // something lower, to more easily verify algorithms
#else
#define OVERHEAT_TEMPERATURE 320 // 340 in original code, 320 = 130* fahrenheit/55* celsius (with calibration)
#endif


///////////////////////////////////
// key points on the light scale //
///////////////////////////////////
#define MAX_LEVEL 1000
#define MAX_LOW_LEVEL 500
#define CURRENT_LEVEL -1
// We will not go below MIN_OVERHEAT_LEVEL even when overheating.  This
//  should only matter when ambient temperature is extremely close to
//  (or above) OVERHEAT_TEMPERATURE.
#define MIN_OVERHEAT_LEVEL 100

#define NOW 1

// turn off strobe... (aka max unsigned long)
#define STROBE_OFF -1

// led constants
#define RLED 0
#define GLED 1

#define LED_OFF 0
#define LED_WAIT 1
#define LED_ON 2

// charging constants
#define CHARGING 1
#define BATTERY 7
#define CHARGED 3

class hexbright {
 public:
  // This is the constructor, it is called when you create a hexbright object.
  hexbright();
  
  // init hardware.
  // put this in your setup().
  static void init_hardware();
  
  // Put update in your loop().  It will block until update_delay has passed.
  static void update();
  
  // When plugged in: turn off the light immediately,
  //   leave the cpu running (as it cannot be stopped)
  // When on battery power: turn off the light immediately,
  //   turn off the cpu in about .5 seconds.
  // Loop will run a few more times, and if your code turns
  //  on the light, shutoff will be canceled. As a result,
  //  if you do not reset your variables you may get weird
  //  behavior after turning the light off and on again in
  // less than .5 seconds.
  static void shutdown();
  
  
#ifdef FREE_RAM
  // freeRam function from: http://playground.arduino.cc/Code/AvailableMemory
  // Arduino uses ~400 bytes of ram, leaving us with 600 to play with
  //  (between function calls (including local variables) and global variables).
  // The library uses < 100 bytes (Accelerometer support adds about 60 bytes).
  //  But, debug mode uses another 100+ bytes.
  //  Between your variables and stack, you should be able to use ~400 safely.
  // Use it if you are experiencing weird crashes.
  //  You can view the value by using code like this:
  //   Serial.println(hb.freeRam());
  // or, if you can't enable debugging:
  //   if(!hb.printing_number())
  //     hb.print_number(hb.freeRam());
  static int freeRam ();
#endif  
  
  
  // go from start_level to end_level over time (in milliseconds)
  // level is from 0-1000.
  // 0 = no light (but still on), 500 = MAX_LOW_LEVEL, MAX_LEVEL=1000.
  // start_level and/or end level can be CURRENT_LEVEL.
  static void set_light(int start_level, int end_level, int time);
  // get light level (before overheat protection adjustment)
  static int get_light_level();
  // get light level (after overheat protection adjustment)
  static int get_safe_light_level();
  // return how long it will be until the light stops changing (in milliseconds).
  // this allows time to be used as a countdown of sorts, between setting lights:
  //  if(hb.light_change_remaining()==0)
  //    hb.set_light(...)
  static int light_change_remaining();

#ifdef STROBE
/////// STROBING ///////

  // Strobing features, limitations, and usage notes:
  //  - not compatible with set_light
  //    (turn off light with set_light(0,0,NOW) before using strobes
  //     turn off strobe with set_strobe_delay(STROBE_OFF) before using set_light)
  //  - the strobe will not drift over time (hooray!)
  //  - strobes will occur within a 25 microsecond window of their update time or not at all
  //     (flicker indicates that we missed our 25 microsecond window)
  //  - strobes occur during the update function; the longer your code takes to execute, the less strobes will occur.
  //  - strobes close to a multiple of 8333 will cause update() to occasionally take slightly 
  //     longer to execute, eventually snap back to the start time, something like this:
  //       8.33 8.33 8.33 8.7 9.4 9.4 9.4 4.83 8.33 (9.4 = 9400 microsecond delay).
  //  - strobe code ignores overheating.  Under most circumstances this should be ok.
  
  // delay between strobes in microseconds.
  //  STROBE_OFF turns off strobing.
  void set_strobe_delay(unsigned long delay);
  // set duration to a value between 50 and 3000.
  //  below 50, you will not see the light
  // 75-400 work well for strobing; 
  //  above 3000 you will lose some accelerometer samples.
  void set_strobe_duration(int duration);
  
  // convenience functions:
  //  example usage: 
  //  set_strobe_fpm(59500);
  //  get_strobe_fpm(); // returns 59523
  //  get_strobe_error(); // returns 703
  //  you are strobing at 59523 fpm, +- 703 fpm.
  // This code requires calibration/tuning
  
  //  fpm accepted is from 0-65k
  //  the higher the fpm, the higher the error.
  void set_strobe_fpm(unsigned int fpm);
  // returns the actual fpm you can currently expect
  unsigned int get_strobe_fpm();
  // returns the current margin of error in fpm
  unsigned int get_strobe_error();
#endif // STROBE

  // Returns true if the button is being pressed
  static BOOL button_pressed();
  // button has just been pressed
  static BOOL button_just_pressed();
  // button has just been released
  static BOOL button_just_released();
  // returns the amount of time (in ms) that the button was last (or is currently being) pressed
  static int button_pressed_time();
  // returns the amount of time (in ms) that the button was last (or is currently being) released
  static int button_released_time();
  
  // led = GLED or RLED,
  // on_time (0-MAXINT) = time in milliseconds before led goes to LED_WAIT state
  // wait_time (0-MAXINT) = time in ms before LED_WAIT state decays to LED_OFF state.
  //   Defaults to 100 ms.
  // brightness (0-255) = brightness of rear led
  //   Defaults to 255 (full brightness)
  // Takes up 16 bytes.
  static void set_led(unsigned char led, int on_time, int wait_time=100, unsigned char brightness=255);
  // led = GLED or RLED
  // returns LED_OFF, LED_WAIT, or LED_ON
  // Takes up 54 bytes.
  static unsigned char get_led_state(unsigned char led);
  // returns the opposite color than the one passed in
  // Takes up 12 bytes.
  static unsigned char flip_color(unsigned char color);
  
  
  // Get the raw thermal sensor reading. Takes up 18 bytes.
  static int get_thermal_sensor();
  // Get the degrees in celsius. I suggest calibrating your sensor, as described
  //  in programs/temperature_calibration. Takes up 60 bytes
  static int get_celsius();
  // Get the degrees in fahrenheit. After calibrating your sensor, you'll need to
  //  modify this as well. Takes up 60 bytes
  static int get_fahrenheit();
  
  // A convenience function that will print the charge state over the led specified
  //  CHARGING = 350 ms on, 350 ms off.
  //  CHARGED = solid on
  //  BATTERY = nothing.
  // If you are using print_number, call it before this function if possible.
  // I recommend the following (if using print_number()):
  //  ...code that may call print number...
  //  if(!printing_number())
  //    print_charge(GLED);
  //  ...end of loop...
  static void print_charge(unsigned char led);
  // returns CHARGING, CHARGED, or BATTERY
  // This reads the charge state twice with a small delay, then returns
  //  the actual charge state.  BATTERY will never be returned if we are
  //  plugged in.
  // Use this if you take actions based on the charge state (example: you
  //  turn on when you stop charging).  Takes up 56 bytes (34+22).
  static unsigned char get_definite_charge_state();
  // returns CHARGING, CHARGED, or BATTERY
  // This reads and returns the charge state, without any verification.
  //  As a result, it may report BATTERY when switching between CHARGED
  //  and CHARGING.
  // Use this if you don't care if the value is sometimes wrong (charging
  //  notification).  Takes up 34 bytes.
  static unsigned char get_charge_state();
  
  
  // prints a number through the rear leds
  // 120 = 1 red flashes, 2 green flashes, one long red flash (0), 2 second delay.
  // the largest printable value is +/-999,999,999, as the left-most digit is reserved.
  // negative numbers begin with a leading long flash.
  static void print_number(long number);
  // currently printing a number
  static BOOL printing_number();
  // reset printing; this immediately terminates the currently printing number.
  static void reset_print_number();

  // reads a value between min_digit to max_digit-1, see hb-examples/numeric_input
  //  Twist the light to change the value.  When the current value changes, 
  //   the green LED will flash, and the number that is currently being printed 
  //   will be reset.  Suppose you are at 2 and you want to go to 5.  Rotate 
  //   clockwise 3 green flashes, and you'll be there.
  //  The current value is printed through the rear leds.
  // Get the result with get_input_digit.
  static void input_digit(unsigned int min_digit, unsigned int max_digit);
  // grab the value that is currently selected (based on twist orientation)
  static unsigned int get_input_digit();
  
#ifdef ACCELEROMETER
  // accepts things like ACC_REG_TILT
  // TILT is now read by default in the private method, at the cost of 12 bytes.
  // using this may cause synchronization errors?
  static unsigned char read_accelerometer(unsigned char acc_reg);
  
  /// interface with the tilt register
  // look at the datasheet page 15 for more details
  //  // http://cache.freescale.com/files/sensors/doc/data_sheet/MMA7660FC.pdf
  // In general, the tilt register is less precise than doing manual
  //  calculations with the vector, but it takes up less space.
  static unsigned char get_tilt_register();
  // return true if the tap flag was set
  static BOOL tapped();
  // return true if the shake flag was set
  static BOOL shaked();
  // returns the tilt orientation:
  //  TILT_UNKNOWN, TILT_HORIZONTAL, TILT_UP, and TILT_DOWN
  //  TILT_UNKNOWN is an extremely rare case (turning on from off, or
  //  the accelerometer turning on from standby).
  static unsigned char get_tilt_orientation();
  // returns tilt rotation.
  //  1 for clockwise, -1 for counterclockwise, 0 for same/unknown.
  // The tilt sensor only changes readings every 90 degrees.  If you need
  //  more precision, look at angle_change
  static char get_tilt_rotation();
  
  
  /// operations that use the vector reading
  // Most units are in 1/100ths of Gs (so 100 = 1G).
  // last two readings have had minor acceleration
  //  The sensor has around .1-.05 Gs of noise, so by default we set tolerance = .1 Gs.
  //  We'll return true if we've had less than that amount of movement in the last two readings.
  static BOOL stationary(int tolerance=10);
  // last reading had non-gravitational acceleration
  //  by default, returns true if the last reading has deviated from 1G by more more
  //  than .5Gs of acceleration.
  static BOOL moved(int tolerance=50);
  
  // returns a value from 100 to -100. 0 is no movement.
  //  This returns lots of noise if we're pointing up or down.
  //  It does have issues if spun too fast or too slow.  I've
  //  found it works well if rotated one-handed.
  static char get_spin();
  //returns the angle between straight down and our current vector
  // returns a value from 0 to 1. 0 == down, 1 == straight up.
  // Multiply by 1.8 to get degrees.  Expect noise of about .1.
  static double difference_from_down();
  // lots of noise < 5 degrees.  Most noise is < 10 degrees
  // noise varies partially based on sample rate, which is not currently configurable
  static double angle_change();
  
  // returns how much acceleration is occurring on a vector, ignoring down.
  //  If no acceleration is occurring, the vector should be close to {0,0,0}.
  static void absolute_vector(int* out_vector, int* in_vector);
  
  
  // Returns the nth vector back from our position.  Currently we only store the last 4 vectors.
  //  0 = most recent reading,
  //  3 = most distant reading.
  // Do not modify the returned vector.
  static int* vector(unsigned char back);
  // Returns our best guess at which way is down.
  // Do not modify the returned vector.
  static int* down();
  
  
  /// vector operations, most operate with 100 = 1G, mathematically.
  // returns a value roughly corresponding to how similar two vectors are.
  static int dot_product(int* vector1, int* vector2);
  // this will give a vector that has experienced no movement, only rotation relative to the two inputs
  static void cross_product(int * out_vector, int* in_vector1, int* in_vector2, double angle_difference);
  // magnitude of a non-normalized vector corresponds to how many Gs we're sensing
  //  The only normalized vector is down.
  static double magnitude(int* vector);
  static void sum_vectors(int* out_vector, int* in_vector1, int* in_vector2);
  static void sub_vectors(int* out_vector, int* in_vector1, int* in_vector2);
  static void copy_vector(int* out_vector, int* in_vector);
  // normalize scales our current vector to 100
  static void normalize(int* out_vector, int* in_vector, double magnitude);
  // angle difference is unusual in that it returns a value from 0-1
  //  (0 = same angle, 1 = opposite).
  static double angle_difference(int dot_product, double magnitude1, double magnitude2);
  
  static void print_vector(int* vector, const char* label);
  
 private: // internal to the library
  // good documentation:
  // http://cache.freescale.com/files/sensors/doc/app_note/AN3461.pdf
  // http://cache.freescale.com/files/sensors/doc/data_sheet/MMA7660FC.pdf
  // 1 ~= .05 Gs (page 28 of the data sheet).
  // reads the x,y,z axes + the tilt register.
  static void read_accelerometer();
  
  static void enable_accelerometer();
  
  // advances the current vector to the next (a place for more data)
  static void next_vector();
  
#ifndef __AVR
 public:
#endif
  // Recalculate down.  If there has been lots of movement, this could easily
  //  be off. But not recalculating down in such cases costs more work, and
  //  even then, we're just guessing.  Overall, a windowed average works fairly
  //  well.
  static void find_down();

  static int low_pass_filter(int last_estimate, int current_reading);
  static int stdev_filter(int last_estimate, int current_reading);
  static int stdev_filter2(int last_estimate, int current_reading);
  static int stdev_filter3(int last_estimate, int current_reading);
  
#endif // ACCELEROMETER
  
 private:
  static void adjust_light();
  static void set_light_level(unsigned long level);
  static void overheat_protection();
  
  static void update_number();
  
  // controls actual led hardware set.
  //  As such, state = HIGH or LOW
  static void _set_led(unsigned char led, unsigned char state);
  static void _led_on(unsigned char led);
  static void _led_off(unsigned char led);
  static void adjust_leds();
  
  static void read_thermal_sensor();

  static void read_button();
  
#ifdef FLASH_CHECKSUM
  // read through flash, return the checksum
  static int flash_checksum();
#endif
///////////////////////////////////////////////
//KLUDGE BECAUSE ARDUINO DOESN'T SUPPORT CLASS VARIABLES/INSTANTIATION
///////////////////////////////////////////////

 public:
  // fake accelerometer read.  This should be in a new, derived class, 
  //  but it can't be because arduino doesn't support class variables
  static void fake_read_accelerometer(int* new_vector);
};
