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

// Pin assignments
#define DPIN_RLED_SW 2 // both red led and switch.  pinMode OUTPUT = led, pinMode INPUT = switch
#define DPIN_GLED 5
#define DPIN_PWR 8
#define DPIN_DRV_MODE 9
#define DPIN_DRV_EN 10
#define APIN_TEMP 0
#define APIN_CHARGE 3


///////////////////////////////////////////////
/////////////HARDWARE INIT, UPDATE/////////////
///////////////////////////////////////////////

const float ms_delay = 8.3333333; // in lock-step with the accelerometer
unsigned long time;

hexbright::hexbright() {
}

void hexbright::init_hardware() {
  // We just powered on! That means either we got plugged
  // into USB, or the user is pressing the power button.
  pinMode(DPIN_PWR, INPUT);
  digitalWrite(DPIN_PWR, LOW);
  // Initialize GPIO
  pinMode(DPIN_RLED_SW, INPUT);
  pinMode(DPIN_GLED, OUTPUT);
  pinMode(DPIN_DRV_MODE, OUTPUT);
  pinMode(DPIN_DRV_EN, OUTPUT);
  digitalWrite(DPIN_DRV_MODE, LOW);
  digitalWrite(DPIN_DRV_EN, LOW);

#if (DEBUG!=DEBUG_OFF)
  // Initialize serial busses
  Serial.begin(9600);
  Wire.begin();
  Serial.println("DEBUG MODE ON");
  if(DEBUG==DEBUG_LIGHT) {
    // do a full light range sweep, (printing all light intensity info)
    set_light(0,1000,1000);
  } else if (DEBUG==DEBUG_TEMP) {
    set_light(0, MAX_LEVEL, NOW);
  } else if (DEBUG==DEBUG_LOOP) {
    // note the use of TIME_MS/ms_delay.
    set_light(0, MAX_LEVEL, 2500/ms_delay);
  }
#endif

#ifdef ACCELEROMETER
  enable_accelerometer();
#endif
  
  time = micros();
}


void hexbright::update() {
  // advance time at the same rate as values are changed in the accelerometer.
  time = time+(1000*ms_delay);
  while (time > micros()) {} // do nothing... (will short circuit once every 70 minutes)
  
  // if we're in debug mode, let us know if our loops are too large
#if (DEBUG!=DEBUG_OFF)
  static int i=0;
  static float avg_loop_time = 0;
  static float last_time = 0;
  avg_loop_time = (avg_loop_time*29 + time-last_time)/30;
#if (DEBUG==DEBUG_LOOP)
  if(!i) {
    Serial.print("Average loop time: ");
    Serial.println(avg_loop_time/1000);
  }
#endif
  if(avg_loop_time/1000>ms_delay+1 && !i) {
    // This may be caused by too much processing for our ms_delay, or by too many print statements (each one takes a few ms)
    Serial.print("WARNING: loop time: ");
    Serial.println(avg_loop_time/1000);
  }
  if(!i) {
    Serial.print("Free ram: ");
	Serial.println(freeRam());
  }
  if (!i)
    i=1000/ms_delay; // display loop output every second
  else
    i--;
  last_time = time;
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
#ifdef ACCELEROMETER
  read_accelerometer();
  find_down();
#endif
  overheat_protection();    
  
  // change light levels as requested
  adjust_light(); 
}

void hexbright::shutdown() {
  pinMode(DPIN_PWR, OUTPUT);
  digitalWrite(DPIN_PWR, LOW);
  digitalWrite(DPIN_DRV_MODE, LOW);
  digitalWrite(DPIN_DRV_EN, LOW);
}


//// freeRam function from: http://playground.arduino.cc/Code/AvailableMemory
int hexbright::freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}



///////////////////////////////////////////////
////////////////LIGHT CONTROL//////////////////
///////////////////////////////////////////////

// Light level must be sufficiently precise for quality low-light brightness and accurate power adjustment at high brightness.
// light level should be converted to logarithmic, square root or cube root values (from lumens), so as to be perceptually linear...
// http://www.candlepowerforums.com/vb/newreply.php?p=3889844
// This is handled inside of set_light_level.


int start_light_level = 0;
int end_light_level = 0;
int change_duration = 0;
int change_done  = 0;

int safe_light_level = MAX_LEVEL;


void hexbright::set_light(int start_level, int end_level, int time) {
// duration ranges from 1-MAXINT
// light_level can be from 0-1000
  if(start_level == CURRENT_LEVEL) {
    start_light_level = get_light_level();
    end_light_level = end_level;
  } else if (end_level == CURRENT_LEVEL) {
    end_light_level = get_light_level();
    start_light_level = start_level;
  } else {
    start_light_level = start_level;
    end_light_level = end_level;
  }

  change_duration = time/ms_delay;
  change_done = 0;
#if (DEBUG==DEBUG_LIGHT)
  Serial.print("Light adjust requested, start level:");
  Serial.println(start_light_level);
#endif

}

int hexbright::get_light_level() {
  if(change_done>=change_duration)
    return end_light_level;
  else 
    return (end_light_level-start_light_level)*((float)change_done/change_duration) +start_light_level; 
}

int hexbright::get_safe_light_level() {
  int light_level = get_light_level();

  if(light_level>safe_light_level)
     return safe_light_level;
  return light_level;
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
  pinMode(DPIN_PWR, OUTPUT);
  digitalWrite(DPIN_PWR, HIGH);
  if(level == 0) {
  // lowest possible power, but still running (DPIN_PWR still high)
    digitalWrite(DPIN_DRV_MODE, LOW);
    analogWrite(DPIN_DRV_EN, 0); 
  }
  else if(level<=500) {
    digitalWrite(DPIN_DRV_MODE, LOW);
    analogWrite(DPIN_DRV_EN, .000000633*(level*level*level)+.000632*(level*level)+.0285*level+3.98);
  } else {
    level -= 500;
    digitalWrite(DPIN_DRV_MODE, HIGH);
    analogWrite(DPIN_DRV_EN, .00000052*(level*level*level)+.000365*(level*level)+.108*level+44.8);
  }  
}

void hexbright::adjust_light() {
  // sets actual light level, altering value to be perceptually linear, based on steven's area brightness (cube root)
  if(change_done<=change_duration) {
    int light_level = hexbright::get_safe_light_level();
    set_light_level(light_level);

    change_done++;
  }
}


  // If the starting temp is much higher than max_temp, it may be a long time before you can turn the light on.
  // this should only happen if: your ambient temperature is higher than max_temp, or you adjust max_temp while it's still hot.
  // Here's an example: ambient temperature is > 
void hexbright::overheat_protection() {
  int temperature = get_thermal_sensor();
  
  safe_light_level = safe_light_level+(OVERHEAT_TEMPERATURE-temperature);
  // min, max levels...
  safe_light_level = safe_light_level > MAX_LEVEL ? MAX_LEVEL : safe_light_level;
  safe_light_level = safe_light_level < 0 ? 0 : safe_light_level;
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
    Serial.print("Current average reading: ");
    Serial.print(printed_temperature);
    Serial.print(" (celsius: ");
    Serial.print(get_celsius());
    Serial.print(") (fahrenheit: ");
    Serial.print(get_fahrenheit());
    Serial.println(")");
    }
#endif

  // if safe_light_level has changed, guarantee a light adjustment:
  if(safe_light_level < MAX_LEVEL) {
#if (DEBUG!=DEBUG_OFF)
    Serial.print("Estimated safe light level: ");
    Serial.println(safe_light_level);
#endif
    change_done  = min(change_done , change_duration);
  }
}

///////////////////////////////////////////////
///////////////////LED CONTROL/////////////////
///////////////////////////////////////////////

#ifdef LED

// >0 = countdown, 0 = change state, -1 = state changed
int led_wait_time[2] = {-1, -1};
int led_on_time[2] = {-1, -1};
byte led_brightness[2] = {0, 0};

void hexbright::set_led(byte led, int on_time, int wait_time, byte brightness) {
#if (DEBUG==DEBUG_LED)
  Serial.println("activate led");
#endif
  led_on_time[led] = on_time/ms_delay;
  led_wait_time[led] = wait_time/ms_delay;
  led_brightness[led] = brightness;
}


byte hexbright::get_led_state(byte led) {
  //returns true if the LED is on
  if(led_on_time[led]>=0) {
    return LED_ON;
  } else if(led_wait_time[led]>0) {
    return LED_WAIT;
  } else {
    return LED_OFF;
  }
}

inline void hexbright::_led_on(byte led) {
  if(led == RLED) { // DPIN_RLED_SW
    analogWrite(DPIN_RLED_SW, led_brightness[RLED]);
    pinMode(DPIN_RLED_SW, OUTPUT);
  } else { // DPIN_GLED
    analogWrite(DPIN_GLED, led_brightness[GLED]);
  }
}

inline void hexbright::_led_off(byte led) {
  if(led == RLED) { // DPIN_RLED_SW
    pinMode(DPIN_RLED_SW, LOW);
    digitalWrite(DPIN_RLED_SW, LOW);
  } else { // DPIN_GLED
   digitalWrite(DPIN_GLED, LOW);
  }
}

inline void hexbright::adjust_leds() {
  // turn off led if it's expired
#if (DEBUG==DEBUG_LED)
  if(led_on_time[GLED]>=0) {
    Serial.print("green on countdown: ");
    Serial.println(led_on_time[GLED]*ms_delay);
  } else if (led_on_time[GLED]<0 && led_wait_time[GLED]>=0) {
    Serial.print("green wait countdown: ");
    Serial.println((led_wait_time[GLED])*ms_delay);
  }
  if(led_on_time[RLED]>=0) {
    Serial.print("red on countdown: ");
    Serial.println(led_on_time[RLED]*ms_delay);
  } else if (led_on_time[RLED]<0 && led_wait_time[RLED]>=0) {
    Serial.print("red wait countdown: ");
    Serial.println((led_wait_time[RLED])*ms_delay);
  }
#endif
  int i=0;
  for(int i=0; i<2; i++) {
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

int time_held = 0;
boolean released = true;

boolean hexbright::button_released() {
  return time_held && released;
}

int hexbright::button_held() {
  return time_held*ms_delay;// && !red_on_time; 
}

void hexbright::read_button() {
  byte button_on = digitalRead(DPIN_RLED_SW);
  if(button_on && released) {
#if (DEBUG==DEBUG_BUTTON)
    Serial.println("Button pressed");
#endif
    released = false;
  } else if (released && time_held) { // we've given a chance for the button press to be read, reset time_held
#if (DEBUG==DEBUG_BUTTON)
    Serial.print("time_held: ");
    Serial.println(time_held*ms_delay);
#endif
    time_held = 0; 
  } else if (!button_on) { // we're off.  don't reset time_held so we can read it one last time
    released = true;
  } else {  // increment time_held one cycle after released has been set, for debouncing
    time_held++;
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
      

byte tilt = 0;
int vectors[] = {0,0,0, 0,0,0, 0,0,0, 0,0,0};
int current_vector = 0;
byte num_vectors = 4;
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
  Wire.beginTransmission(ACC_ADDRESS);
  Wire.write(config, sizeof(config));
  Wire.endTransmission();

  // Enable accelerometer
  byte enable[] = {ACC_REG_MODE, 0x01};  // Mode: active!
  Wire.beginTransmission(ACC_ADDRESS);
  Wire.write(enable, sizeof(enable));
  Wire.endTransmission();
 
 // pinMode(DPIN_ACC_INT,  INPUT);
 // digitalWrite(DPIN_ACC_INT,  HIGH);
}

void hexbright::read_accelerometer() {
  /*unsigned long time = 0;
  if((millis()-init_time)>*/
  // advance which vector is considered the first
  next_vector();
  while(1) {
    Wire.beginTransmission(ACC_ADDRESS);
    Wire.write(ACC_REG_XOUT);          // starting with ACC_REG_XOUT, 
    Wire.endTransmission(false);
    Wire.requestFrom(ACC_ADDRESS, 4);  // read 4 registers (X,Y,Z), TILT
    for(int i=0; i<4; i++) {
      if (!Wire.available())
        continue;
      char tmp = Wire.read();
      if(tmp & 0x40) // Bx1xxxxxx, re-read per data sheet page 14
        continue;
      if(i==3){ //read tilt register
	    tilt = tmp;
	  } else { // read vector
        if(tmp & 0x20) // Bxx1xxxxx, it's negative
          tmp |= 0xC0; // extend to B111xxxxx
        vectors[current_vector+i] = tmp*(100/21.3); // 1~=.05 Gs(datasheet page 28)
	  }
    }
    break;
  }
}

byte hexbright::read_accelerometer(byte acc_reg) {
  if (!digitalRead(DPIN_ACC_INT)) {
    Wire.beginTransmission(ACC_ADDRESS);
    Wire.write(acc_reg);
    Wire.endTransmission(false);       // End, but do not stop!
    Wire.requestFrom(ACC_ADDRESS, 1);  
    return Wire.read();
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
  normalize(down_vector, down_vector, magnitudes);
}

/// tilt register interface

byte hexbright::get_tilt_register() {
  return tilt;
}

boolean hexbright::tapped() {
  return tilt & 0x20;
}

boolean hexbright::shaked() {
  return tilt & 0x80;
}

byte hexbright::get_tilt_orientation() {
  byte tmp = tilt & (0x1F | 0x03); // filter out the tap/shake registers, and the back/front register
  tmp = tmp>>2; // shift us all the way to the right
  // PoLa: 5, 6 = horizontal, 1 = up, 2 = down, 0 = unknown
  if(tmp & 0x04) 
    return TILT_HORIZONTAL;
  return tmp;
}

char hexbright::get_tilt_rotation() {
  static byte last = 0;
  byte current  = tilt & 0x1F; // filter out tap/shake registers
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

boolean hexbright::stationary(int tolerance) {
  // low acceleration vectors
  return abs(magnitude(vector(0))-100)<tolerance && abs(magnitude(vector(1))-100)<tolerance; 
}

boolean hexbright::moved(int tolerance) {
  return abs(magnitude(vector(0))-100)>tolerance;
}

char hexbright::get_spin() {
  // quick formula:
  //(atan2(vector(1)[0], vector(1)[2]) - atan2(vector(0)[0], vector(0)[2]))*32;  
  // we cache the last position, because it takes up less space.
  static char last_position = 0;
  // 2*PI*32 = 200
  char position = atan2(vector(0)[0], vector(0)[2])*32;  
  char tmp = last_position-position;
  last_position = position;
  return tmp;
}

/// VECTOR TOOLS
int* hexbright::vector(byte back) {
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
      axes_rotation[i] = (in_vector1[(i+1)%3]*in_vector2[(i+2)%3] \
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

void hexbright::print_vector(int* vector, char* label) {
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
byte _color = GLED;
int print_wait_time = 0;

#if (defined(LED) && defined(PRINT_NUMBER))
boolean hexbright::printing_number() {
  return _number || print_wait_time; 
}

void hexbright::update_number() {
  if(_number>0) { // we have something to do...
#ifdef DEBUG
    if(DEBUG==DEBUG_NUMBER) {
      static int last_printed = 0;
      if(last_printed != _number) {
        last_printed = _number;
        Serial.print("number remaining (read from right to left): ");
        Serial.println(_number);
      }
    }
#endif
    if(!print_wait_time) {
      if(_number==1) { // minimum delay between printing numbers
        print_wait_time = 2500/ms_delay;
        _number = 0;
        return;
      } else {
        print_wait_time = 300/ms_delay; 
      }
      if(_number/10*10==_number) {
#if (DEBUG==DEBUG_NUMBER)
        Serial.println("zero"); 
#endif
//        print_wait_time = 500/ms_delay; 
        set_led(_color, 400); 
      } else {
        set_led(_color, 120);
        _number--;
      }
      if(_number && !(_number%10)) { // next digit?
        print_wait_time = 600/ms_delay;
        _color = flip_color(_color);
        _number = _number/10;
      }
    }
  } 

  if(print_wait_time) {
    print_wait_time--;
  } 
}

byte hexbright::flip_color(byte color) {
  return (color+1)%2;
}


void hexbright::print_number(long number) {
  // reverse number (so it prints from left to right)
  boolean negative = false;
  if(number<0) {
    number = 0-number;
    negative = true; 
  }
  _color = GLED;
  _number=1; // to guarantee printing when dealing with trailing zeros (100 can't be stored as 001, use 1001)
  while(number>0) {
    _number = _number * 10 + (number%10); 
    number = number/10;
    _color = flip_color(_color);
  }
  if(negative) {
    set_led(flip_color(_color), 500);
    print_wait_time = 600/ms_delay;
  }
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
  
  thermal_sensor_value = analogRead(APIN_TEMP);
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

int hexbright::get_thermal_sensor() {
  return thermal_sensor_value;
}



///////////////////////////////////////////////
//////////////////CHARGING/////////////////////
///////////////////////////////////////////////

byte hexbright::get_charge_state() {
  int charge_value = analogRead(APIN_CHARGE);
#if (DEBUG==DEBUG_CHARGE)
  Serial.print("Current charge reading: ");
  Serial.println(charge_value);
#endif
  // <128 charging, >768 charged, battery
  if(charge_value<128)
    return CHARGING;
  else if (charge_value>768)
    return CHARGED;
  return BATTERY;
}

// reading twice costs us 28 bytes, but improves reliability.
// The root problem is when the charge value goes from <128 to >768 (or the 
//  reverse, from topping off), it passes through the middle range.  If we 
//  read at the wrong time, we can get a BATTERY value while we are still 
//  plugged in.
// Reading twice with a sufficient delay, we can guarantee that our state is correct.
byte hexbright::get_definite_charge_state() {
  byte val1 = get_charge_state();
  // do something that will take some time...
  // delayMicroseconds costs an extra 20 bytes (because nowhere else is it called)
  // If other code needs delayMicroseconds, switch to it.
  //delayMicroseconds(30); 
  read_thermal_sensor(); // delay a little...
  byte val2 = get_charge_state();
  // BATTERY & CHARGING = CHARGING, BATTERY & CHARGED = CHARGED, CHARGED & CHARGING = CHARGING
  // In essence, only return the middle value (BATTERY) if two reads report the same thing.
  return val1 & val2;
}