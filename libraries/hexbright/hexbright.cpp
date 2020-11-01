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


#include "hexbright.h"
#include <limits.h>

#ifndef __AVR // we're not compiling for arduino (probably testing), use these stubs
#include "pc_stubs.h"
#else
#include "read_adc.h"
#include "../digitalWriteFast/digitalWriteFast.h"
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


///////////////////////////////////////////////
/////////////HARDWARE INIT, UPDATE/////////////
///////////////////////////////////////////////

const float update_delay = 8.3333333; // in lock-step with the accelerometer
unsigned long continue_time;

#ifdef STROBE
unsigned long next_strobe = STROBE_OFF;
unsigned long strobe_delay = 0;
int strobe_duration = 100;
#endif

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
  
  continue_time = micros();
}

word loopCount;
void hexbright::update() {
  unsigned long now;
  loopCount++;

#if (DEBUG==DEBUG_LOOP)
  unsigned long start_time=micros();
#endif


#ifdef STROBE  
  while (true) {
    do {
      now = micros();
    } while (next_strobe > now && // not ready for strobe
	     continue_time > now); // not ready for update

    if (next_strobe <= now) {
      if (now - next_strobe <26) {
	digitalWriteFast(DPIN_DRV_EN, HIGH);
	delayMicroseconds(strobe_duration);
	digitalWriteFast(DPIN_DRV_EN, LOW);
      }
      next_strobe += strobe_delay;
    }
    if(continue_time <= now) {
      if(strobe_delay>update_delay && // we strobe less than once every 8333 microseconds
	 next_strobe-continue_time < 4000) // and the next strobe is within 4000 microseconds (may occur before we return)
	continue;
      else
	break;
    }
  } // do nothing... (will short circuit once every 70 minutes (micros maxint))
#else
    do {
      now = micros();
    } while ((signed long)(continue_time - now) > 0); // not ready for update
#endif  

  // if we're in debug mode, let us know if our loops are too large
#if (DEBUG!=DEBUG_OFF && DEBUG!=DEBUG_PRINT)
  static int i=0;
#if (DEBUG==DEBUG_LOOP)
  static unsigned long last_time = 0;
  if(!i) {
    Serial.print("Time used: ");
    Serial.print(start_time-last_time);
    Serial.println("/8333");
  }
  last_time = now;
#endif
  if(now-continue_time>5000 && !i) {
    // This may be caused by too much processing for our update_delay, or by too many print statements)
    //  If you're triggering this, your button and light will react more slowly, and some accelerometer
    //  data is being missed.
    Serial.println("WARNING: code is too slow");
  }
  if (!i)
    i=1000/update_delay; // display loop output every second
  else
    i--;
#endif
  
  
  // power saving modes described here: http://www.atmel.com/Images/2545s.pdf
  //run overheat protection, time display, track battery usage
  
#ifdef LED
  // regardless of desired led state, turn it off so we can read the button
  _led_off(RLED);
  delayMicroseconds(50); // let the light stabilize...
  read_button();
  // turn on (or off) the leds, if appropriate
  adjust_leds();
#ifdef PRINT_NUMBER
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

  // advance time at the same rate as values are changed in the accelerometer.
  //  advance continue_time here, so the first run through short-circuits, 
  //  meaning we will read hardware immediately after power on.
  continue_time = continue_time+(int)(1000*update_delay);
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

void hexbright::set_light_level(unsigned long level) {
  // LOW 255 approximately equals HIGH 48/49.  There is a color change.
  // Values < 4 do not provide any light.
  // I don't know about relative power draw.
  
  // look at linearity_test.ino for more detail on these algorithms.
  
#if (DEBUG==DEBUG_LIGHT)
  Serial.print("light level: ");
  Serial.println(level);
#endif
  digitalWriteFast(DPIN_PWR, HIGH);
  if(level == 0) {
    // lowest possible power, but cpu still running (DPIN_PWR still high)
    digitalWriteFast(DPIN_DRV_MODE, LOW);
    analogWrite(DPIN_DRV_EN, 0);
  } else if(level == OFF_LEVEL) {
    // power off (DPIN_PWR LOW)
    digitalWriteFast(DPIN_PWR, LOW);
    digitalWriteFast(DPIN_DRV_MODE, LOW);
    analogWrite(DPIN_DRV_EN, 0);
  } else { 
    byte value;
    if(level<=500) {
      digitalWriteFast(DPIN_DRV_MODE, LOW);
      value = (byte)(.000000633*(level*level*level)+.000632*(level*level)+.0285*level+3.98);
    } else {
      level -= 500;
      digitalWriteFast(DPIN_DRV_MODE, HIGH);
      value = (byte)(.00000052*(level*level*level)+.000365*(level*level)+.108*level+44.8);
    }
    analogWrite(DPIN_DRV_EN, value);
  }
}

void hexbright::adjust_light() {
  // sets actual light level, altering value to be perceptually linear, based on steven's area brightness (cube root)
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


#ifdef STROBE

///////////////STROBE CONTROL//////////////////

void hexbright::set_strobe_delay(unsigned long delay) {
  strobe_delay = delay;
  next_strobe = micros()+strobe_delay;
}

void hexbright::set_strobe_duration(int duration) {
  strobe_duration = duration;
}

void hexbright::set_strobe_fpm(unsigned int fpm) {
  set_strobe_delay(60000000/fpm);
}

unsigned int hexbright::get_strobe_fpm() {
  return 60000000 / (strobe_delay/8*8);
}

unsigned int hexbright::get_strobe_error() {
  // 90000000 because we have an error of 3*8 microseconds; 1.5 above, 1.5 below
  return 90000000 / ((strobe_delay/8)*8) - 90000000 / ((strobe_delay/8+1)*8);
}

#endif

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

byte clickState;
#define CLICK_OFF 0
#define CLICK_ACTIVE 1
#define CLICK_WAIT 2

byte clickCount;
word max_click_time;
void hexbright::config_click_count(word click_time) {
  max_click_time = click_time;
  clickState=0;
}

char hexbright::click_count() {
  switch(clickState) {
  case CLICK_OFF:
    if(button_just_pressed()) {
      // click just started
      clickState = CLICK_ACTIVE;
      clickCount=0;
      //Serial.println("Clicking just started");
    }
    break;
  case CLICK_ACTIVE:   
    if(button_just_released()) {
      if(button_pressed_time() > max_click_time) {
	// button held to long
	//Serial.println("Click held too long");
	clickState = CLICK_OFF;
      } else {
	// click is counted
	clickState = CLICK_WAIT;
	clickCount++;
	//Serial.println("Click counted");
      }
    }
    break;
  case CLICK_WAIT:
    // button is released for long enough, we're done clicking
    if(button_released_time() > max_click_time) {
      clickState = CLICK_OFF;
      //Serial.print("Click finished: "); Serial.println((int)clickCount);
      return clickCount;
    } else if(button_pressed()) {
      // move back to active state
      clickState = CLICK_ACTIVE;
      //Serial.println("Click active");
    }
  }
  return -127;
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
//////////////////UTILITIES////////////////////
///////////////////////////////////////////////

long _number = 0;
unsigned char _color = GLED;
int print_wait_time = 0;

#if (defined(LED) && defined(PRINT_NUMBER))
BOOL hexbright::printing_number() {
  return _number || print_wait_time;
}

void hexbright::reset_print_number() {
  _number = 1;
  print_wait_time = 0;
}

void hexbright::update_number() {
  if(_number>0) { // we have something to do...
#if (DEBUG==DEBUG_NUMBER)
    static int last_printed = 0;
    if(last_printed != _number) {
      last_printed = _number;
      Serial.print("number remaining (read from right to left): ");
      Serial.println(_number);
    }
#endif
    if(!print_wait_time) {
      if(_number==1) { // minimum delay between printing numbers
        print_wait_time = 2500/update_delay;
        _number = 0;
        return;
      } else {
        print_wait_time = 300/update_delay;
      }
      if(_number/10*10==_number) {
#if (DEBUG==DEBUG_NUMBER)
        Serial.println("zero");
#endif
        //        print_wait_time = 500/update_delay;
        set_led(_color, 400);
      } else {
        set_led(_color, 120);
        _number--;
      }
      if(_number && !(_number%10)) { // next digit?
        print_wait_time = 600/update_delay;
        _color = flip_color(_color);
        _number = _number/10;
      }
    }
  }
  
  if(print_wait_time) {
    print_wait_time--;
  }
}

unsigned char hexbright::flip_color(unsigned char color) {
  return (color+1)%2;
}


void hexbright::print_number(long number) {
  // reverse number (so it prints from left to right)
  BOOL negative = false;
  if(number<0) {
    number = 0-number;
    negative = true;
  }
  _color = GLED;
  _number=1; // to guarantee printing when dealing with trailing zeros (100 can't be stored as 001, use 1001)
  do {
    _number = _number * 10 + (number%10);
    number = number/10;
    _color = flip_color(_color);
  }  while(number>0);
  if(negative) {
    set_led(flip_color(_color), 500);
    print_wait_time = 600/update_delay;
  }
}


#ifdef ACCELEROMETER
//// Numeric entry
static unsigned int read_value = 0;

unsigned int hexbright::get_input_digit() {
  return read_value;
}

void hexbright::input_digit(unsigned int min_digit, unsigned int max_digit) {
  unsigned int tmp2 = 999 - atan2(vector(0)[0], vector(0)[2])*159 - 500; // scale from 0-999, counterclockwise = higher
  tmp2 = (tmp2*(max_digit-min_digit))/1000+min_digit;
  if(tmp2 == read_value) {
    if(!printing_number()) {
      print_number(tmp2);
    }
  } else {
    reset_print_number();
    set_led(GLED,100);
  }
  read_value = tmp2; 
}
#endif // ACCELEROMETER
#endif // (defined(LED) && defined(PRINT_NUMBER))

void hexbright::print_power() {
  print_charge(GLED);
  if (low_voltage_state() && get_led_state(RLED) == LED_OFF) {
    set_led(RLED,50,1000);
  }
}


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

void hexbright::print_charge(unsigned char led) {
  unsigned char charge_state = get_charge_state();
  if(charge_state == CHARGING && get_led_state(led) == LED_OFF) {
    set_led(led, 350, 350);
  } else if (charge_state == CHARGED) {
    set_led(led,50);
  }
}


///////////////////////////////////////////////
//////////////////SHUTDOWN/////////////////////
///////////////////////////////////////////////

void hexbright::shutdown() {
#if (DEBUG!=DEBUG_OFF)
  Serial.println("don't use shutdown, use set_light(,OFF_LEVEL,)");
#endif
  set_light(MAX_LOW_LEVEL, OFF_LEVEL, NOW);
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
