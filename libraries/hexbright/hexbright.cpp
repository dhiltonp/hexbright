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

int ms_delay;
unsigned long last_time;

hexbright::hexbright(int update_delay_ms) {
  ms_delay = update_delay_ms;
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

#ifdef DEBUG
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
  if(ms_delay<9) {
#ifdef DEBUG
    Serial.println("Warning, ms_delay too low for accelerometer.  Adjusting to 9 ms.");
#endif
    ms_delay = 9;
  }
  enable_accelerometer();
#endif
  
  last_time = millis();
}


void hexbright::update() {
  unsigned long time;
  do {
    time = millis();
  } while (time-last_time < ms_delay);


  // loop 200? 60? times per second?
  // The point is, we want light adjustments to be constant regardless of how much processing is going on.
#ifdef DEBUG
  static int i=0;
  static float avg_loop_time = 0;
  avg_loop_time = (avg_loop_time*29 + time-last_time)/30;
  if(DEBUG==DEBUG_LOOP && !i) {
    Serial.print("Average loop time: ");
    Serial.println(avg_loop_time);
  }
  if(avg_loop_time>ms_delay+1 && !i) {
    // This may be caused by too much processing for our ms_delay, or by too many print statements (each one takes a few ms)
    Serial.print("WARNING: loop time: ");
    Serial.println(avg_loop_time);
  }
  if (!i)
    i=1000/ms_delay; // display loop output every second
  else
    i--;
#endif

  last_time = time;
  // power saving modes described here: http://www.atmel.com/Images/2545s.pdf
  //run overheat protection, time display, track battery usage
  read_button();
  read_thermal_sensor(); // takes about .2 ms to execute (fairly long, relative to the other steps)
#ifdef ACCELEROMETER
  read_accelerometer_vector();
#endif
  overheat_protection();    
  update_number();
  
  // change light levels as requested
  adjust_light(); 
  adjust_leds();
}

void hexbright::shutdown() {
  pinMode(DPIN_PWR, OUTPUT);
  digitalWrite(DPIN_PWR, LOW);
  digitalWrite(DPIN_DRV_MODE, LOW);
  digitalWrite(DPIN_DRV_EN, LOW);
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
    start_light_level = get_safe_light_level();
    end_light_level = end_level;
  } else {
    start_light_level = start_level;
    end_light_level = end_level;
  }

  change_duration = time/ms_delay;
  change_done = 0;
#ifdef DEBUG
  if (DEBUG == DEBUG_LIGHT) {
    Serial.print("Light adjust requested, start level:");
    Serial.println(start_light_level);
  }
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

#ifdef DEBUG
  if (DEBUG == DEBUG_LIGHT) {
    Serial.print("light level: ");
    Serial.println(level);
  }
#endif
  pinMode(DPIN_PWR, OUTPUT);
  digitalWrite(DPIN_PWR, HIGH);
  if(level == 0) {
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
#ifdef DEBUG
  if(DEBUG==DEBUG_TEMP) {
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
  }
#endif

  // if safe_light_level has changed, guarantee a light adjustment:
  if(safe_light_level < MAX_LEVEL) {
#ifdef DEBUG
    Serial.print("Estimated safe light level: ");
    Serial.println(safe_light_level);
#endif
    change_done  = min(change_done , change_duration);
  }
}

///////////////////////////////////////////////
/////////////BUTTON & LED CONTROL//////////////
///////////////////////////////////////////////
// because the red LED and button share the same pin,
// we need to share some logic, hence the merged section.
// the switch cannot receive input while the red led is on.

// >0 = countdown, 0 = change state, -1 = state changed
int led_on_time[2] = {-1, -1};
boolean released = true; //button starts down

//////////////////////LED//////////////////////

// >0 = countdown, 0 = change state, -1 = state changed
int led_wait_time[2] = {-1, -1};

void hexbright::set_led(byte led, int on_time, int wait_time) {
#ifdef DEBUG
  if(DEBUG==DEBUG_BUTTON) {
    Serial.println("activate led");
  }
#endif DEBUG
  if(on_time>0) {
    _set_led(led,HIGH);
  } else {
    _set_led(led,LOW);
  }
  led_on_time[led] = on_time/ms_delay;
  led_wait_time[led] = wait_time/ms_delay;
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

 void hexbright::_set_led(byte led, byte state) {
  // this state is HIGH/LOW
  // keep this debug section and the next section in sync
#ifdef DEBUG 
  if(DEBUG==DEBUG_BUTTON) {
    if(led == RLED) { // DPIN_RLED_SW
      if (!released) {
        Serial.println("can't set red led, switch is down");
      } else {
        if(state == HIGH) {
          Serial.println("Red LED on");
        } else {
          Serial.println("Red LED off");
        }
      }
    } else { // DPIN_GLED
      if(state==HIGH)
        Serial.println("Green LED on");
      else
        Serial.println("Green LED off");
    }
  }
#endif
  // keep in sync with the previous debug section.
  if(led == RLED) { // DPIN_RLED_SW
    if (released) {
      if(state == HIGH) {
        digitalWrite(DPIN_RLED_SW, state);
        pinMode(DPIN_RLED_SW, OUTPUT);
      } else {
        pinMode(DPIN_RLED_SW, state);
        digitalWrite(DPIN_RLED_SW, LOW);
      }
    }
 } else { // DPIN_GLED
   digitalWrite(DPIN_GLED, state);
 }
}

void hexbright::adjust_leds() {
  // turn off led if it's expired
#ifdef DEBUG
  if(DEBUG==DEBUG_BUTTON) {
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
  }
#endif
  int i=0;
  for(int i=0; i<2; i++) {
    if(led_on_time[i]==0) {
      _set_led(i, LOW);
    }
    if(led_on_time[i]>=0) {
      led_on_time[i]--;
    } else if (led_wait_time[i]>=0) {
      led_wait_time[i]--;
    }
 }
}

/////////////////////BUTTON////////////////////

int time_held = 0;

boolean hexbright::button_released() {
  return time_held && released;
}

int hexbright::button_held() {
  return time_held*ms_delay;// && !red_on_time; 
}

void hexbright::read_button() {
  if(led_on_time[RLED] >= 0) { // led is still on, wait
#ifdef DEBUG
   if(DEBUG==DEBUG_BUTTON) {
      Serial.println("Red LED is active, no switch for you");
   }
#endif
    return;
  }
  byte button_on = digitalRead(DPIN_RLED_SW);
  if(button_on) {
#ifdef DEBUG
    if(DEBUG==DEBUG_BUTTON && released)
      Serial.println("Button pressed");
#endif
    time_held++; 
    released = false;
  } else if (released && time_held) { // we've given a chance for the button press to be read, reset time_held
#ifdef DEBUG
   if(DEBUG==DEBUG_BUTTON)
      Serial.print("time_held: ");
      Serial.println(time_held*ms_delay);
#endif
    time_held = 0; 
  } else {
    released = true;
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

boolean using_accelerometer = false;



int vectors[] = {0,0,0, 0,0,0};
int* new_vector = vectors;
int* old_vector = vectors+3;
int zaxis[3] = {1,0,0};


double magnitude = 0;
double dp = 0;
double angle_change = 0;
double axes_rotation[] = {0,0,0};


double hexbright::get_angle_change() {
  return angle_change;
}

double hexbright::get_dp() {
  return dp;
}

double* hexbright::get_axes_rotation() {
  return axes_rotation;
}

double hexbright::get_gs() {
  return magnitude;
}



double hexbright::jab_detect(float sensitivity) {
#ifdef DEBUG
  if(DEBUG==DEBUG_ACCEL)
    Serial.println((int)sensitivity);
#endif
  //  if(abs(magnitude-1)>.2)
  if(angle_change>15 && abs(magnitude-1)>.4)
    return 1;
  return 0;
}


void hexbright::print_vector(int* vector) {
  for(int i=0; i<3; i++) {
    Serial.print(vector[i]); 
    Serial.print("/");
  }
  Serial.println("vector");
}
void hexbright::print_accelerometer() {
  print_vector(old_vector);
  print_vector(new_vector);
  for(int i=0; i<3; i++) {
    Serial.print(axes_rotation[i]); 
    Serial.print("/");
  }
  Serial.println("axes of rotation");
  Serial.print(angle_change);
  Serial.println(" (degrees)");
  Serial.print("Magnitude (acceleration in Gs): ");
  Serial.println(magnitude);
  Serial.print("Dp: ");
  Serial.println(dp);
}

double hexbright::dot_product(int* vector1, int* vector2) {
  int sum = 0;
  for(int i=0;i<3;i++) {
    sum+=vector1[i]*vector2[i];
  } 
  return sum/(21.3*21.3); // convert to Gs (datasheet appendix C)
}

double hexbright::get_magnitude(int* vector) {
  int result = 0;
  for(int i=0; i<3;i++) {
   result += vector[i]*vector[i];
  }
  return sqrt(result/(21.3*21.3)); // convert to Gs (datasheet appendix C)
}


int hexbright::convert_axis_number(byte value) {
  if((value>>7)%2) { // Alert bit set, invalid data...
    Serial.println("Invalid data, returning 0");
    return 100; // out of range
  }
  int val = value<<10;
  return val>>10;
}

void hexbright::read_accelerometer_vector() {
  // swap first vector
  int * tmp_vector = new_vector;
  new_vector = old_vector;
  old_vector = tmp_vector;

  if (!digitalRead(DPIN_ACC_INT)) {
    Wire.beginTransmission(ACC_ADDRESS);
    Wire.write(ACC_REG_XOUT);          // starting with ACC_REG_XOUT, 
    Wire.endTransmission(false);
    Wire.requestFrom(ACC_ADDRESS, 3);  // read 3 registers (X,Y,Z)
    for(int i=0; i<3; i++) {//read into our vector
      // TODO: check for number being invalid
      new_vector[i]=convert_axis_number(Wire.read());
    }
  } else {
    for(int i=0; i<3; i++) {//read into x, then y, then z
      new_vector[i]=0;
    }
  }

  // calculate Gs
  dp = dot_product(old_vector, new_vector);
  //  gs = 

  // calculate angle change
  // equation 45 from http://cache.freescale.com/files/sensors/doc/app_note/AN3461.pdf
  double old_magnitude = magnitude;
  magnitude = get_magnitude(new_vector);
  angle_change = acos(dp/magnitude*old_magnitude);

  // calculate instantaneous rotation around axes
  // equation 47 from http://cache.freescale.com/files/sensors/doc/app_note/AN3461.pdf
  for(int i=0; i<3; i++) {
    axes_rotation[i] = (new_vector[(i+1)%3]*old_vector[(i+2)%3] \
                        - new_vector[(i+2)%3]*old_vector[(i+1)%3]);
    axes_rotation[i] /= 21.3; // convert to Gs (datasheet appendix C);
    axes_rotation[i] /= magnitude*old_magnitude;
    //    axes_rotation[i] /= asin(angle_change);
  }

  // change angle_change from radians to degrees
  angle_change *= 180/3.14159;  

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


void hexbright::enable_accelerometer() {
  byte sample_rate = 6; //111
  for(int i=0; i<=6; i++) {
    if(1000/ms_delay> (1<<i)) {
      //       1000/12000>1, leave sample_rate at 6
      //       1000/200>1, sample_rate=5(110)
      //       1000/200>2, sample_rate=4(101)
      //       1000/200>4, sample_rate=3(100)
      //       1000/200>8, sample_rate=3(011)
      //       1000/200>16, sample_rate=3(010)
      //       1000/200>32, sample_rate=3(001)
      //       1000/200>64, sample_rate=3(111)
      //       1000/200>128, sample_rate=3(111)
      sample_rate = 6-i;
    }
  }
#ifdef DEBUG
  if(DEBUG==DEBUG_ACCEL)
    Serial.println((int)sample_rate);
#endif

  
  // Configure accelerometer
  byte config[] = {
    ACC_REG_INTS,  // First register (see next line)
    0xE4,  // Interrupts: shakes, taps
    0x00,  // Mode: not enabled yet
    sample_rate,  // Sample rate: 120 Hz (see datasheet page 19)
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

void hexbright::disable_accelerometer() {
  
}

#endif

///////////////////////////////////////////////
//////////////////UTILITIES////////////////////
///////////////////////////////////////////////

long _number = 0;
byte _color = GLED;
int print_wait_time = 0;


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
#ifdef DEBUG
        if(DEBUG==DEBUG_NUMBER) {
         Serial.println("zero"); 
        }
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
  // intersection with 0: 52.5 = (40C-0C)/(275-153)*153
  // readings obtained with DEBUG_TEMP
  return thermal_sensor_value * ((40.05-0)/(275-153)) - 50; 
}

int hexbright::get_fahrenheit() {
  return get_celsius()*18/10+32;
}

int hexbright::get_thermal_sensor() {
  return thermal_sensor_value;
}



///////////////////////////////////////////////
//////////////////CHARGING/////////////////////
///////////////////////////////////////////////

byte hexbright::get_charge_state() {
  int charge_value = analogRead(APIN_CHARGE);
#ifdef DEBUG
  if (DEBUG == DEBUG_CHARGE) {
    Serial.print("Current charge reading: ");
    Serial.println(charge_value);
    Serial.print("Current charge state: ");
    Serial.println(int((charge_value-129.0)/640+1));
  }
#endif
  // <128 charging, >768 charged, battery
  return int((charge_value-129.0)/640+1);
}
