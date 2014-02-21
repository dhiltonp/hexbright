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

#ifndef HEXBRIGHT_H
#define HEXBRIGHT_H

#ifdef __AVR // we're compiling for arduino
#include <Arduino.h>
#include "twi.h"
#include "../digitalWriteFast/digitalWriteFast.h"
#define BOOL boolean
#else
#define BOOL bool
#endif


// Pin assignments
#define DPIN_RLED_SW 2 // both red led and switch.  pinMode OUTPUT = led, pinMode INPUT = switch
#define DPIN_GLED 5
#define DPIN_PWR 8
#define DPIN_DRV_MODE 9
#define DPIN_DRV_EN 10
#define APIN_TEMP 0
#define APIN_CHARGE 3
#define APIN_BAND_GAP 14


/// Some space-saving options
#define LED // comment out save 786 bytes if you don't use the rear LEDs
#define ACCELEROMETER //comment out to save 1500 bytes if you don't need the accelerometer
#define FLASH_CHECKSUM // comment out to save 56 bytes when in debug mode
#define FREE_RAM // comment out to save 146 bytes when in debug mode


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
// Some debug modes set the light.  Your control code may reset it, causing weird flashes at startup.
#define DEBUG_OFF 0 // no extra code is compiled in
#define DEBUG_PRINT 1 // initialize printing only
#define DEBUG_ON 2 // initialize printing, print if certain things are obviously wrong
#define DEBUG_LOOP 3 // tells you how long your code is taking to execute (
#define DEBUG_LIGHT 4 // Light control
#define DEBUG_TEMP 5  // temperature safety
#define DEBUG_BUTTON 6 // button presses - you may experience some flickering LEDs if enabled
#define DEBUG_LED 7 // rear LEDs - you may get flickering LEDs with this enabled
#define DEBUG_ACCEL 8 // accelerometer
#define DEBUG_NUMBER 9 // number printing utility
#define DEBUG_CHARGE 10 // charge state
#define DEBUG_PROGRAM 11 // use this to enable/disable print statements in the program rather than the library

#if (DEBUG==DEBUG_TEMP)
#define OVERHEAT_TEMPERATURE 265 // something lower, to more easily verify algorithms
#else
#define OVERHEAT_TEMPERATURE 320 // 340 in original code, 320 = 130* fahrenheit/55* celsius (with calibration)
#endif

#define UPDATE_DELAY 8.33333333


///////////////////////////////////
// key points on the light scale //
///////////////////////////////////
#define MAX_LEVEL 1000
#define MAX_LOW_LEVEL 500
#define OFF_LEVEL -1
#define CURRENT_LEVEL -2
// We will not go below MIN_OVERHEAT_LEVEL even when overheating.  This
//  should only matter when ambient temperature is extremely close to
//  (or above) OVERHEAT_TEMPERATURE.
#define MIN_OVERHEAT_LEVEL 100

#define NOW 1


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

// Bit manipulation macros
#define BIT_CHECK(reg,bit) (reg & (1<<bit))
#define BIT_SET(reg,bit) reg |= (1<<bit)
#define BIT_CLEAR(reg,bit) reg &= ~(1<<bit)
#define BIT_TOGGLE(reg,bit) reg ^= (1<<bit)

class hexbright {
 public:
  // This is the constructor, it is called when you create a hexbright object.
  hexbright();
  
  // init hardware.
  // put this in your setup().
  static void init_hardware();
  
  // Put update in your loop().  It will block until update_delay has passed.
  static void update();
  
  
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
  // level is from -2 to 1000.
  // -2 = CURRENT_LEVEL (whatever the light is currently at), can be used as start_level or end_level
  // -1 = OFF_LEVEL, no light, turn off (if on battery power)
  // 0 = no light
  // 500 = MAX_LOW_LEVEL, max low power mode
  // 1000 = MAX_LEVEL, max high power mode
  // max change time is about 4.5 minutes ((2^15-1)*8.333 milliseconds).
  //  I have had weird issues when passing in 3*60*1000, 180000 works fine though.
  // Specific algorithms can be customized via #defines (see set_light_level.h)
  static void set_light(int start_level, int end_level, long time);
  // get light level (before overheat protection adjustment)
  static int get_light_level();
  // get light level (after overheat protection adjustment)
  static int get_max_light_level();
  // return how long it will be until the light stops changing (in milliseconds).
  // this allows time to be used as a countdown of sorts, between setting lights:
  //  if(hb.light_change_remaining()==0)
  //    hb.set_light(...)
  static int light_change_remaining();


  // Button debouncing is handled inside the library.
  // Returns true if the button is being pressed.
  static BOOL button_pressed();
  // button has just been pressed
  static BOOL button_just_pressed();
  // button has just been released
  static BOOL button_just_released();
  // If the button is currently pressed, returns the amount of time the button has been pressed.
  // If the button is currently released, returns the duration of the last button press.
  //  The time is in milliseconds.
  static int button_pressed_time();
  // If the button is currently pressed or was just released, returns the duration between the previous two button presses.
  // If the button is currently released, returns the amount of time since the button was released.
  //  The time is in milliseconds.
  static int button_released_time();
  // The next time update() is run, the button says it is pressed, as if it physically happened.
  //  Used in init_hardware to initialize the button state to pressed
  //  so that we can catch very fast initial presses.
  static void press_button();
  
  // led = GLED or RLED,
  // on_time (0-MAXINT) = time in milliseconds before led goes to LED_WAIT state
  // wait_time (0-MAXINT) = time in ms before LED_WAIT state decays to LED_OFF state.
  //   Defaults to 100 ms.
  // brightness (0-255) = brightness of rear led. note that rled brightness only has 2 bits of resolution and has visible flicker at the lowest setting.
  //   Defaults to 255 (full brightness)
  // Takes up 16 bytes.
  static void set_led(unsigned char led, int on_time, int wait_time=100, unsigned char brightness=255);
  // led = GLED or RLED
  // returns LED_OFF, LED_WAIT, or LED_ON
  // Takes up 54 bytes.
  static unsigned char get_led_state(unsigned char led);
  
  
  // Get the raw thermal sensor reading. Takes up 18 bytes.
  static int get_thermal_sensor();
  // Get the degrees in celsius. I suggest calibrating your sensor, as described
  //  in programs/temperature_calibration. Takes up 60 bytes
  static int get_celsius();
  // Get the degrees in fahrenheit. After calibrating your sensor, you'll need to
  //  modify this as well. Takes up 60 bytes
  static int get_fahrenheit();

  // returns the raw avr voltage.  
  //  This is not equivalent to the battery voltage, and should be stable unless the 
  //  battery is very low or the voltage regulator is having problems.
  static int get_avr_voltage();
  // returns true if we are in a low voltage state (unable to go to max brightness)
  //  This triggers based on irregular power, which should only occur if we're
  //  out of juice (due to voltage regulation)
  //  This may be useful if you want your light to flash when running low on power
  static BOOL low_voltage_state();


  
  // returns CHARGING, CHARGED, or BATTERY
  static unsigned char get_charge_state();
  
  
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
  // Multiply by 180 to get degrees.  Expect noise of about .1 (15-20 degrees).
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
  static void apply_max_light_level();
  static void detect_overheating();
  static void detect_low_battery();
  
  // controls actual led hardware set.
  //  As such, state = HIGH or LOW
  static void _set_led(unsigned char led, unsigned char state);
  static void _led_on(unsigned char led);
  static void _led_off(unsigned char led);
  static void adjust_leds();
  
  static void read_thermal_sensor();
  static void read_charge_state();
  static void read_avr_voltage();

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

#endif // HEXBRIGHT_H

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/////////////////////////////  hexbright.cpp  /////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



#ifdef BUILD_HACK

#include "update_spin.h"
#include <limits.h>

#ifndef __AVR // we're not compiling for arduino (probably testing), use these stubs
#include "pc_stubs.h"
#else
#include "read_adc.h"
#include "../digitalWriteFast/digitalWriteFast.h"
#endif


/// set #define-based options

// select the appropriate set_light_level based on #defines...
#include "set_light_level.h"

// default to no debug mode if no override has been set
#ifndef DEBUG
#define DEBUG DEBUG_OFF
#endif


///////////////////////////////////////////////
/////////////HARDWARE INIT, UPDATE/////////////
///////////////////////////////////////////////

const float update_delay = UPDATE_DELAY; // in lock-step with the accelerometer


hexbright::hexbright() {
}

#ifdef FLASH_CHECKSUM
int hexbright::flash_checksum() {
  int checksum = 0;
  for(int i=0; i<16384; i++) {
    checksum+=pgm_read_byte(i);
  }
  return checksum;
}
#endif

void hexbright::init_hardware() {
  // These next 8 commands are for reference and cost nothing,
  //  as we are initializing the values to their default state.
  pinModeFast(DPIN_PWR, OUTPUT);
  digitalWriteFast(DPIN_PWR, LOW);
  pinModeFast(DPIN_RLED_SW, INPUT);
  pinModeFast(DPIN_GLED, OUTPUT);
  pinModeFast(DPIN_DRV_MODE, OUTPUT);
  pinModeFast(DPIN_DRV_EN, OUTPUT);
  digitalWriteFast(DPIN_DRV_MODE, LOW);
  digitalWriteFast(DPIN_DRV_EN, LOW);
  
#if (DEBUG!=DEBUG_OFF)
  // Initialize serial busses
  Serial.begin(9600);
  twi_init(); // for accelerometer
  Serial.println("DEBUG MODE ON");
#endif

#if (DEBUG!=DEBUG_OFF && DEBUG!=DEBUG_PRINT)
#ifdef FREE_RAM
  Serial.print("Ram available: ");
  Serial.print(freeRam());
  Serial.println("/1024 bytes");
#endif
#ifdef FLASH_CHECKSUM
  Serial.print("Flash checksum: ");
  Serial.println(flash_checksum());
#endif
#endif //(DEBUG!=DEBUG_OFF && DEBUG!=DEBUG_PRINT)
  
#ifdef ACCELEROMETER
  enable_accelerometer();
#endif
  
  // was this power on from battery? if so, it was a button press, even if it was too fast to register.
  read_charge_state();
  if(get_charge_state()==BATTERY)
    press_button();
}

word loopCount;
void hexbright::update() {
  loopCount++;

  update_spin();
 
  // power saving modes described here: http://www.atmel.com/Images/2545s.pdf
  //run overheat protection, time display, track battery usage
  
#ifdef LED
  // regardless of desired led state, turn it off so we can read the button
  _led_off(RLED);
  delayMicroseconds(50); // let the light stabilize...
  read_button();
  // turn on (or off) the leds, if appropriate
  adjust_leds();
#ifdef PRINT_NUMBER_H
  update_number();
#endif
#else
  read_button();
#endif
  
  read_thermal_sensor(); // takes about .2 ms to execute (fairly long, relative to the other steps)
  read_charge_state();
  read_avr_voltage();

#ifdef ACCELEROMETER
  read_accelerometer();
  find_down();
#endif
  detect_overheating();
  detect_low_battery();
  apply_max_light_level();
  
  // change light levels as requested
  adjust_light();

}


#ifdef FREE_RAM
//// freeRam function from: http://playground.arduino.cc/Code/AvailableMemory
int hexbright::freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
#endif


///////////////////////////////////////////////
///////////////////Filters/////////////////////
///////////////////////////////////////////////

#ifdef ACCELEROMETER

inline int hexbright::low_pass_filter(int last_estimate, int current_reading) {
  // The sum of these two constant multipliers (which equals the divisor), 
  //  should not exceed 210 (to avoid integer overflow)
  // the individual values selected do effect the resulting sketch size :/
  return (2*last_estimate + 3*current_reading)/5;
}

inline int hexbright::stdev_filter(int last_estimate, int current_reading) {
  float stdev = 3.3; // our standard deviation due to noise (pre calculated using accelerometer data at rest)
  int diff = -abs(last_estimate-current_reading);
  float deviation = diff/stdev;
  float probability = exp(deviation)/1.25; // 2.5 ~= M_SQRT2PI, /2 because cdf is only one way
  // exp by itself takes 400-500 bytes.  This isn't good.
  return probability*last_estimate + (1-probability)*current_reading;
}


inline int hexbright::stdev_filter2(int last_estimate, int current_reading) {
  // uses 1/deviation^2 for our cdf approximation
  float stdev = 3.5; // our standard deviation due to noise (pre calculated using accelerometer data at rest)
  int diff = abs(last_estimate-current_reading);
  float deviation = diff/stdev;
  float probability;
  if(deviation>1) // estimate where we fall on the cdf
    probability = 1/(deviation*deviation);
  else
    probability = .65;
  // exp by itself takes 400-500 bytes.  This isn't good.
  return probability*last_estimate + (1-probability)*current_reading;
}

inline int hexbright::stdev_filter3(int last_estimate, int current_reading) {
  // uses 1/deviation^2 for our cdf approximation, switched to ints
  int stdev = 3.5*10; // our standard deviation due to noise (pre calculated using accelerometer data at rest)
  int diff = abs(last_estimate-current_reading)*10;
  // differences < 4 have a deviation of 0
  int deviation = diff/stdev;
  int probability;
  if(deviation>1) // estimate where we fall on the cdf
    probability = 100/(deviation*deviation);
  else
    probability = 70;
  return (probability*last_estimate + (100-probability)*current_reading)/100;
}

#endif // ACCELEROMETER

///////////////////////////////////////////////
////////////////LIGHT CONTROL//////////////////
///////////////////////////////////////////////

// Light level must be sufficiently precise for quality low-light brightness and accurate power adjustment at high brightness.
// light level should be converted to logarithmic, square root or cube root values (from lumens), so as to be perceptually linear...
// http://www.candlepowerforums.com/vb/newreply.php?p=3889844
// This is handled inside of set_light_level.


int start_light_level = 0;
int end_light_level = OFF_LEVEL; // go to OFF_LEVEL once change_duration expires (unless set_light overrides)
int change_duration = 5000/update_delay; // stay on for 5 seconds
int change_done = 0;

int max_light_level = MAX_LEVEL;


void hexbright::set_light(int start_level, int end_level, long time) {
  // duration ranges from 1-MAXINT
  // light_level can be from 0-1000
  int current_level = get_light_level();
  start_light_level = start_level == CURRENT_LEVEL ? current_level : start_level;
  end_light_level   = end_level   == CURRENT_LEVEL ? current_level : end_level;
  
  change_duration = ((float)time)/update_delay;
  change_done = 0;
#if (DEBUG==DEBUG_LIGHT)
  Serial.print("Light adjust requested, start level: ");
  Serial.println(start_light_level);
  Serial.print("Over ");
  Serial.print(change_duration);
  Serial.println(" updates");
#endif
  
}

int hexbright::get_light_level() {
  if(change_done>=change_duration)
    return end_light_level;
  else
    return (end_light_level-start_light_level)*((float)change_done/change_duration) +start_light_level;
}

int hexbright::get_max_light_level() {
  int light_level = get_light_level();
  
  if(light_level>max_light_level)
    return max_light_level;
  return light_level;
}

int hexbright::light_change_remaining() {
  // change_done ends up at -1, add one to counter
  //  return (change_duration-change_done+1)*update_delay;
  int tmp = change_duration-change_done;
  if(tmp<=0)
    return 0;
  return tmp*update_delay;
}

void hexbright::adjust_light() {
  if(change_done<=change_duration) {
    int light_level = hexbright::get_max_light_level();
    set_light_level(light_level);
    
    change_done++;
  }
}

void hexbright::apply_max_light_level() {
  // if max_light_level has changed, guarantee a light adjustment:
  // the second test guarantees that we won't turn on if we are
  //  overheating and just shut down
  if(max_light_level < MAX_LEVEL && get_light_level()>MIN_OVERHEAT_LEVEL) {
#if (DEBUG!=DEBUG_OFF && DEBUG!=DEBUG_PRINT)
    Serial.print("Max light level: ");
    Serial.println(max_light_level);
#endif
    change_done  = change_done < change_duration ? change_done : change_duration;
  }
}


///////////////////////////////////////////////
///////////////////LED CONTROL/////////////////
///////////////////////////////////////////////

#ifdef LED

// >0 = countdown, 0 = change state, -1 = state changed
int led_wait_time[2] = {-1, -1};
int led_on_time[2] = {-1, -1};
unsigned char led_brightness[2] = {0, 0};
byte rledMap[4] = {0b0001, 0b0101, 0b0111, 0b1111};

void hexbright::set_led(unsigned char led, int on_time, int wait_time, unsigned char brightness) {
#if (DEBUG==DEBUG_LED)
  Serial.println("activate led");
#endif
  led_on_time[led] = on_time/update_delay;
  led_wait_time[led] = wait_time/update_delay;
  led_brightness[led] = brightness;
}


unsigned char hexbright::get_led_state(unsigned char led) {
  //returns true if the LED is on
  if(led_on_time[led]>=0) {
    return LED_ON;
  } else if(led_wait_time[led]>0) {
    return LED_WAIT;
  } else {
    return LED_OFF;
  }
}

inline void hexbright::_led_on(unsigned char led) {
  if(led == RLED) { // DPIN_RLED_SW
    pinModeFast(DPIN_RLED_SW, OUTPUT);

    byte l = rledMap[led_brightness[RLED]>>6];
    byte r = 1<<(loopCount & 0b11);
    if(l & r) {
      digitalWriteFast(DPIN_RLED_SW, HIGH);
    } else {
      digitalWriteFast(DPIN_RLED_SW, LOW);
    }
  } else { // DPIN_GLED
    analogWrite(DPIN_GLED, led_brightness[GLED]);
  }
}

inline void hexbright::_led_off(unsigned char led) {
  if(led == RLED) { // DPIN_RLED_SW
    digitalWriteFast(DPIN_RLED_SW, LOW);
    pinModeFast(DPIN_RLED_SW, INPUT);
  } else { // DPIN_GLED
    // digitalWriteFast doesn't work when set to a non-digital value, 
	//  using analogWrite. digitalWrite could be used, but would increase 
	//  complexity in porting to straight AVR.
    analogWrite(DPIN_GLED, LOW);
  }
}

inline void hexbright::adjust_leds() {
  // turn off led if it's expired
#if (DEBUG==DEBUG_LED)
  if(led_on_time[GLED]>=0) {
    Serial.print("green on countdown: ");
    Serial.println(led_on_time[GLED]*update_delay);
  } else if (led_on_time[GLED]<0 && led_wait_time[GLED]>=0) {
    Serial.print("green wait countdown: ");
    Serial.println((led_wait_time[GLED])*update_delay);
  }
  if(led_on_time[RLED]>=0) {
    Serial.print("red on countdown: ");
    Serial.println(led_on_time[RLED]*update_delay);
  } else if (led_on_time[RLED]<0 && led_wait_time[RLED]>=0) {
    Serial.print("red wait countdown: ");
    Serial.println((led_wait_time[RLED])*update_delay);
  }
#endif

  int i=0;
  for(i=0; i<2; i++) {
    if(led_on_time[i]>0) {
      _led_on(i);
      led_on_time[i]--;
    } else if(led_on_time[i]==0) {
      _led_off(i);
      led_on_time[i]--;
    } else if (led_wait_time[i]>=0) {
      led_wait_time[i]--;
    }
  }
}

#endif


///////////////////////////////////////////////
/////////////////////BUTTON////////////////////
///////////////////////////////////////////////

#define BUTTON_FILTER 7 // 00000111 (this is applied at read time)
#define BUTTON_JUST_OFF(state) (state==4)  // 100 = just off
#define BUTTON_JUST_ON(state) (state==1)   // 001 = just on
#define BUTTON_STILL_OFF(state) (state==0) // 000 = still off
// 010, 011, 101, 110, 111 = still on
#define BUTTON_STILL_ON(state) !(BUTTON_JUST_OFF(state) | BUTTON_JUST_ON(state) | BUTTON_STILL_OFF(state))
#define BUTTON_ON(state) (state & 3) // either of right most bits is on: 00000011 (not 100 or 000)
#define BUTTON_OFF(state) !BUTTON_ON(state)

// button state could hold a history of the last 8 switch values, 1 == on, 0 == off.
// for debouncing, we only need the most recent 3, so BUTTON_FILTER=7, 00000111
// This is applied during the read process.
unsigned char button_state = 0;

unsigned long time_last_pressed = 0; // the time that button was last pressed
unsigned long time_last_released = 0; // the time that the button was last released

byte press_override = false;

void hexbright::press_button() {
  press_override = true;
}

BOOL hexbright::button_pressed() {
  return BUTTON_ON(button_state);
}

BOOL hexbright::button_just_pressed() {
  return BUTTON_JUST_ON(button_state);
}

BOOL hexbright::button_just_released() {
  return BUTTON_JUST_OFF(button_state);
}

int hexbright::button_pressed_time() {
  if(BUTTON_ON(button_state) || BUTTON_JUST_OFF(button_state)) {
    return millis()-time_last_pressed;
  } else {
    return time_last_released - time_last_pressed;
  }
}

int hexbright::button_released_time() {
  if(BUTTON_ON(button_state)) {
    return time_last_pressed-time_last_released;
  } else {
    return millis()-time_last_released;
  }
}

void hexbright::read_button() {
  if(BUTTON_JUST_OFF(button_state)) {
    // we update time_last_released before the read, so that the very first time through after a release, 
	//  button_released_time() returns the /previous/ button_released_time.
    time_last_released=millis();
#if (DEBUG==DEBUG_BUTTON)
    Serial.println("Button just released");
    Serial.print("Time spent pressed (ms): ");
    Serial.println(time_last_released-time_last_pressed);
#endif
  }
  
  /* READ THE BUTTON!!!
    button_state = button_state << 1;                            // make space for the new value
    button_state = button_state | digitalReadFast(DPIN_RLED_SW); // add the new value
    button_state = button_state & BUTTON_FILTER;                 // remove excess values */
  // Doing the three commands above on one line saves 2 bytes.  We'll take it!
  byte read_value = digitalReadFast(DPIN_RLED_SW);
  if(press_override) {
    read_value = 1;
	press_override = false;
  }
  button_state = ((button_state<<1) | read_value) & BUTTON_FILTER;
  
  if(BUTTON_JUST_ON(button_state)) {
    time_last_pressed=millis();
#if (DEBUG==DEBUG_BUTTON)
    Serial.println("Button just pressed");
    Serial.print("Time spent released (ms): ");
    Serial.println(time_last_pressed-time_last_released);
#endif
  }
}

///////////////////////////////////////////////
////////////////ACCELEROMETER//////////////////
///////////////////////////////////////////////

#ifdef ACCELEROMETER

// return degrees of movement?
// Possible things to work with:
//MOVE_TYPE, value returned from a successful detect_movement
#define ACCEL_NONE   0 // nothing
#define ACCEL_TWIST  1 // return degrees - light axis remains constant
#define ACCEL_TURN   2 // return degrees - light axis changes
#define ACCEL_DROP   3 // return change of velocity - period of no acceleration before impact?
#define ACCEL_TAP    4 // return change of velocity - acceleration before impact

// I considered operating with bytes to save space, but the savings were
//  offset by still needing to do /some/ things as ints.  I've not completely
//  tested which is more compact, but preliminary work suggests difficulty
//  in the implementation with little to no benefit.


unsigned char tilt = 0;
int vectors[] = {0,0,0, 0,0,0, 0,0,0, 0,0,0};
int current_vector = 0;
unsigned char num_vectors = 4;
int down_vector[] = {0,0,0};

/// SETUP/MANAGEMENT

void hexbright::enable_accelerometer() {
  // Configure accelerometer
  byte config[] = {
    ACC_REG_INTS,  // First register (see next line)
    0xE4,  // Interrupts: shakes, taps
    0x00,  // Mode: not enabled yet
    0x00,  // Sample rate: 120 Hz (see datasheet page 19)
    0x0F,  // Tap threshold
    0x05   // Tap debounce samples
  };
  twi_writeTo(ACC_ADDRESS, config, sizeof(config), true /*wait*/, true /*send stop*/);
  
  // Enable accelerometer
  byte enable[] = {ACC_REG_MODE, 0x01};  // Mode: active!
  twi_writeTo(ACC_ADDRESS, enable, sizeof(enable), true /*wait*/, true /*send stop*/);
  
  // pinModeFast(DPIN_ACC_INT,  INPUT);
  // digitalWriteFast(DPIN_ACC_INT,  HIGH);
}

void hexbright::read_accelerometer() {
  /*unsigned long time = 0;
    if((millis()-init_time)>*/
  // advance which vector is considered the first
  next_vector();
  char read=0;
  while(read!=4) {
    byte reg = ACC_REG_XOUT;
    twi_writeTo(ACC_ADDRESS, &reg, sizeof(reg), true /*wait*/, true /*send stop*/);
	byte acc_data[4];
	twi_readFrom(ACC_ADDRESS, acc_data, sizeof(acc_data), true /*send stop*/);
    read = 0;
    int i = 0;
    for(i; i<4; i++) {
      char tmp = acc_data[i];
	  if (tmp & 0x40) { // Bx1xxxxxx,
		// invalid data, re-read per data sheet page 14
		continue; // continue, so we finish flushing the read buffer.
	  }
      if(i==3){ //read tilt register
        tilt = tmp;
      } else { // read vector
        if(tmp & 0x20) // Bxx1xxxxx, it's negative
          tmp |= 0xC0; // extend to B111xxxxx
		vectors[current_vector+i] = stdev_filter3(vector(1)[i], tmp*(100/21.3));
      }
	  read++; // successfully read.
    }
  }
}

unsigned char hexbright::read_accelerometer(unsigned char acc_reg) {
  if (!digitalReadFast(DPIN_ACC_INT)) {
    byte acc_data;
    twi_writeTo(ACC_ADDRESS, &acc_reg, sizeof(acc_reg), true /*wait*/, true /*send stop*/);
    twi_readFrom(ACC_ADDRESS, &acc_data, sizeof(acc_data), true /*send stop*/);
    return acc_data;
  }
  return 0;
}


inline void hexbright::find_down() {
  // currently, we're just averaging the last four data points.
  //  Down is the strongest constant acceleration we experience
  //  (assuming we're not dropped).  Heuristics to only find down
  //  under specific circumstances have been tried, but they only
  //  tell us when down is less certain, not where it is...
  copy_vector(down_vector, vector(0)); // copy first vector to down
  double magnitudes = magnitude(vector(0));
  for(int i=1; i<num_vectors; i++) { // go through, summing everything up
    int* vtmp = vector(i);
    sum_vectors(down_vector, down_vector, vtmp);
    magnitudes+=magnitude(vtmp);
  }
  normalize(down_vector, down_vector, magnitudes!=0 ? magnitudes : 1);
}

/// tilt register interface

unsigned char hexbright::get_tilt_register() {
  return tilt;
}

BOOL hexbright::tapped() {
  return tilt & 0x20;
}

BOOL hexbright::shaked() {
  return tilt & 0x80;
}

unsigned char hexbright::get_tilt_orientation() {
  unsigned char tmp = tilt & (0x1F | 0x03); // filter out the tap/shake registers, and the back/front register
  tmp = tmp>>2; // shift us all the way to the right
  // PoLa: 5, 6 = horizontal, 1 = up, 2 = down, 0 = unknown
  if(tmp & 0x04)
    return TILT_HORIZONTAL;
  return tmp;
}

char hexbright::get_tilt_rotation() {
  static unsigned char last = 0;
  unsigned char current  = tilt & 0x1F; // filter out tap/shake registers
  // 21,22,26,25 (in order, rotating when horizontal)
  switch(current ) {
  case 21: // 10101
    current  = 1;
    break;
  case 22: // 10110
    current  = 2;
    break;
  case 26: // 11010
    current  = 3;
    break;
  case 25: // 11001
    current  = 4;
    break;
  default: // we can't determine orientation with this reading
    last = 0;
    return 0;
  }
  
  if(last==0) { // previous reading wasn't usable
    last = current;
    return 0;
  }
  
  // we have two valid values, calculate!
  char retval = last-current;
  last = current;
  if(retval*retval>1) { // switching from a 4 to a 1 or vice-versa
    retval = -(retval%2);
  }
#if (DEBUG==DEBUG_ACCEL)
  if(retval!=0) {
    Serial.print("tilt rotation: ");
    Serial.println((int)retval);
  }
#endif
  return retval;
}

/// some sample functions using vector operations

double hexbright::angle_change() {
  int* vec1 = vector(0);
  int* vec2 = vector(1);
  return angle_difference(dot_product(vec1, vec2),
                          magnitude(vec1),
                          magnitude(vec2));
}

void hexbright::absolute_vector(int* out_vector, int* in_vector) {
  sub_vectors(out_vector, in_vector, down_vector);
}

double hexbright::difference_from_down() {
  int light_axis[3] = {0, -100, 0};
  return (angle_difference(dot_product(light_axis, down_vector), 100, 100));
}

BOOL hexbright::stationary(int tolerance) {
  // low acceleration vectors
  return abs(magnitude(vector(0))-100)<tolerance && abs(magnitude(vector(1))-100)<tolerance;
}

BOOL hexbright::moved(int tolerance) {
  return abs(magnitude(vector(0))-100)>tolerance;
}

char hexbright::get_spin() {
  // quick formula:
  //(atan2(vector(1)[0], vector(1)[2]) - atan2(vector(0)[0], vector(0)[2]))*32;
  // we cache the last position, because it takes up less space.
  static char last_spin = 0;
  // 2*PI*32 = 200
  return atan2(down()[0], down()[2])*32 - atan2(vector(0)[0], vector(0)[2])*32;
  char spin = atan2(vector(0)[0], vector(0)[2])*32;
  if(abs(last_spin)-10<0) {

  }
  
  return spin;
}

/// VECTOR TOOLS
int* hexbright::vector(unsigned char back) {
  return vectors+((current_vector/3+back)%num_vectors)*3;
}

int* hexbright::down() {
  return down_vector;
}

void hexbright::next_vector() {
  current_vector=((current_vector/3+(num_vectors-1))%num_vectors)*3;
}

double hexbright::angle_difference(int dot_product, double magnitude1, double magnitude2) {
  // multiply to account for 100 being equal to one for our vectors
  double tmp = dot_product*100/(magnitude1*magnitude2);
  return acos(tmp)/3.14159;
}

int hexbright::dot_product(int* vector1, int* vector2) {
  // the max value sum could hit is technically > 2^16.
  //  However, the only instance where this is possible is if both vectors have
  //  experienced maximum acceleration on all axes - with 4 of the 6 being the max in the negative
  //  direction (-32 min, 31 max), (31*100/21.3) = 145, (-32*100/21.3) = -150
  //  3*(150^2) = 67500 (not safe)
  //  (150*150)+(150*150)+(145*145) = 66025 (not safe)
  //  (150*150)+(150*145)+(145*145) = 65275 (safe)
  //  3*(145^2) = 63075 (safe)
  // Given the difficulty of triggering this condition, I'm using an int.
  //  This could also be avoided by /100 inside the loop, costing 12 bytes.
  //  We should revisit this decision when drop detect code has been written.
  // An alternate solution would be to convert using READING*100/21.65, giving
  //  a max of 147.  3*(147^2) = 64827 (safe).  A max of 148 would be safe so
  //  long as no more than 5 values are -32.
  unsigned int sum = 0; // max value is about 3*(150^2), a tad over the maximum unsigned int
  for(int i=0;i<3;i++) {
    //sum+=vector1[i]*vector2[i]/100; // avoids overflow, but costs space
    sum+=vector1[i]*vector2[i];
  }
  return sum/100; // small danger of overflow
}

void hexbright::cross_product(int * axes_rotation,
                              int* in_vector1,
                              int* in_vector2,
                              double angle_difference) {
  for(int i=0; i<3; i++) {
    axes_rotation[i] = (in_vector1[(i+1)%3]*in_vector2[(i+2)%3]         \
                        - in_vector1[(i+2)%3]*in_vector2[(i+1)%3]);
    //axes_rotation[i] /= magnitude(in_vector1)*magnitude(in_vector2);
    //axes_rotation[i] /= asin(angle_difference*3.14159);
  }
}

double hexbright::magnitude(int* vector) {
  // This has a small possibility of failure, see the comments at the start
  //  of dot_product.  A long would cost another 28 bytes, a double 42.
  //  A cheaper solution (costing 20 bytes) is to divide the squaring by 100,
  //  then multiply the sqrt by 10 (commented out).
  unsigned int result = 0;
  for(int i=0; i<3;i++) {
    result += vector[i]*vector[i]; // vector[i]*vector[i]/100;
  }
  return sqrt(result); // sqrt(result)*10;
}

void hexbright::normalize(int* out_vector, int* in_vector, double magnitude) {
  for(int i=0; i<3; i++) {
    // normalize to 100, not 1
    out_vector[i] = in_vector[i]/magnitude*100;
  }
}

void hexbright::sum_vectors(int* out_vector, int* in_vector1, int* in_vector2) {
  for(int i=0; i<3;i++) {
    out_vector[i] = in_vector1[i]+in_vector2[i];
  }
}

void hexbright::sub_vectors(int* out_vector, int* in_vector1, int* in_vector2) {
  for(int i=0; i<3;i++) {
    out_vector[i] = in_vector1[i]-in_vector2[i];
  }
}

void hexbright::copy_vector(int* out_vector, int* in_vector) {
  for(int i=0; i<3;i++) {
    out_vector[i] = in_vector[i];
  }
}

void hexbright::print_vector(int* vector, const char* label) {
#if (DEBUG!=DEBUG_OFF)
  for(int i=0; i<3; i++) {
    Serial.print(vector[i]);
    Serial.print("/");
  }
  Serial.println(label);
#endif
}

#endif


///////////////////////////////////////////////
////////////////TEMPERATURE////////////////////
///////////////////////////////////////////////

int thermal_sensor_value = 0;
void hexbright::read_thermal_sensor() {
  // do not call this directly.  Call get_temperature()
  // read temperature setting
  // device data sheet: http://ww1.microchip.com/downloads/en/devicedoc/21942a.pdf
  
  thermal_sensor_value = read_adc(APIN_TEMP);
}

int hexbright::get_thermal_sensor() {
  return thermal_sensor_value;
}

int hexbright::get_celsius() {
  // 0C ice water bath for 20 minutes: 153.
  // 40C water bath for 20 minutes (measured by medical thermometer): 275
  // intersection with 0: 50 = (40C-0C)/(275-153)*153
  
  // 40.05 is to force the division to floating point.  The extra parenthesis are to
  //  tell the compiler to pre-evaluate the expression.
  return thermal_sensor_value * ((40.05-0)/(275-153)) - 50;
}


int hexbright::get_fahrenheit() {
  //return get_celsius()*18/10+32;
  // algebraic form of (get_celsius' formula)*18/10+32
  // I was lazy and pasted (x*((40.05-0)/(275-153)) - 50)*18/10+32 into wolfram alpha
  return .590902*thermal_sensor_value-58;
}

// If the ambient temperature is above your max temp, your light is going to be pretty dim...

void hexbright::detect_overheating() {
  unsigned int temperature = get_thermal_sensor();
  
  max_light_level = max_light_level+(OVERHEAT_TEMPERATURE-temperature);
  // min, max levels...
  max_light_level = max_light_level > MAX_LEVEL ? MAX_LEVEL : max_light_level;
  max_light_level = max_light_level < MIN_OVERHEAT_LEVEL ? MIN_OVERHEAT_LEVEL : max_light_level;
#if (DEBUG==DEBUG_TEMP)
  static float printed_temperature = 0;
  static float average_temperature = -1;
  if(average_temperature < 0) {
    average_temperature = temperature;
    Serial.println("Have you calibrated your thermometer?");
    Serial.println("Instructions are in get_celsius.");
  }
  average_temperature = (average_temperature*4+temperature)/5;
  if (abs(printed_temperature-average_temperature)>1) {
    printed_temperature = average_temperature;
    Serial.print(millis());
    Serial.print(" ms, average reading: ");
    Serial.print(printed_temperature);
    Serial.print(" (celsius: ");
    Serial.print(get_celsius());
    Serial.print(") (fahrenheit: ");
    Serial.print(get_fahrenheit());
    Serial.println(")");
  }
#endif
}


///////////////////////////////////////////////
////////////////AVR VOLTAGE////////////////////
///////////////////////////////////////////////

int band_gap_reading = 0;
int lowest_band_gap_reading = 1000;

void hexbright::read_avr_voltage() {
  band_gap_reading = read_adc(APIN_BAND_GAP);
  if(get_charge_state()==BATTERY)
    lowest_band_gap_reading = band_gap_reading < lowest_band_gap_reading ? band_gap_reading : lowest_band_gap_reading;
}

int hexbright::get_avr_voltage() {
  // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  // this is the only place we actually convert to voltage, reducing the space used for most programs.
  return ((long)1023*1100) / band_gap_reading;
}

BOOL hexbright::low_voltage_state() {
  static BOOL low = false;
  // lower band gap value corresponds to a higher voltage, trigger 
  //  low voltage state if band gap value goes too high.
  // I have a value of 2 for this to work (with a 150 ms delay in read_adc).
  //  tighter control means earlier detection of low battery state
  if (band_gap_reading > lowest_band_gap_reading+2) {
    low = true;
  }
  return low;
}

void hexbright::detect_low_battery() {
  if (low_voltage_state() == true && max_light_level>500) {
    max_light_level = 500;
  }
}


///////////////////////////////////////////////
//////////////////CHARGING/////////////////////
///////////////////////////////////////////////

// we store the last two charge readings in charge_state, and due to the specific
//  values chosen for the #defines, we greatly decrease the possibility of returning
//  BATTERY when we are still connected over USB.  This costs 14 bytes.

// BATTERY is the median value, which is only returned if /both/ halves are BATTERY.
//  If one of the two values is CHARGING, we return CHARGING
//  If one of the two values is CHARGED (and neither CHARGING), we return CHARGED
//  Otherwise, BATTERY is both values and is returned
unsigned char charge_state = BATTERY;

void hexbright::read_charge_state() {
  unsigned int charge_value = read_adc(APIN_CHARGE);
#if (DEBUG==DEBUG_CHARGE)
  Serial.print("Current charge reading: ");
  Serial.println(charge_value);
#endif
  // <128 charging, >768 charged, battery
  charge_state <<= 4;
  if(charge_value<128)
    charge_state += CHARGING;
  else if (charge_value>768)
    charge_state += CHARGED;
  else
    charge_state += BATTERY;
}

unsigned char hexbright::get_charge_state() {
  // see more details on how this works at the top of this section
  return charge_state & (charge_state>>4);
}


///////////////////////////////////////////////
//KLUDGE BECAUSE ARDUINO DOESN'T SUPPORT CLASS VARIABLES/INSTANTIATION
///////////////////////////////////////////////
#ifdef ACCELEROMETER
void hexbright::fake_read_accelerometer(int* new_vector) {
  next_vector();
  for(int i=0; i<3; i++) {
    vector(0)[i] = stdev_filter3(vector(1)[i], new_vector[i]);
    //vector(0)[i] = new_vector[i];
  }
}
#endif // ACCELEROMETER

#endif //BUILD_HACK
