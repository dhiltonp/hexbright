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
  
  enable_accelerometer();
  
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
  read_accelerometer();
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


void hexbright::set_light(int start_level, int end_level, int updates) {
// duration ranges from 1-MAXINT
// light_level can be from 0-1000
  if(start_level == CURRENT_LEVEL) {
    start_light_level = get_safe_light_level();
    end_light_level = end_level;
  } else {
    start_light_level = start_level;
    end_light_level = end_level;
  }

  change_duration = updates;
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
  //pinMode(DPIN_PWR, OUTPUT);
  //digitalWrite(DPIN_PWR, HIGH);*/
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

int led_delay = 100;
int red_time = 0;
boolean released = false; //button starts down

//////////////////////LED//////////////////////

int green_time = 0;
/*
void set_led(byte led, int time) {
// led = DPIN_GLED or DPIN_RLED_SW,
// time = cycles before led is turned off (0=now)
#ifdef DEBUG
  if(DEBUG==DEBUG_BUTTON)
    Serial.println("activate led");
#endif DEBUG
  if(time==0) {
    _set_led(led,LOW);  
  } else {
    _set_led(led,HIGH);  
  }
  if(led==DPIN_RLED_SW) {
    red_time = time + led_delay/ms_delay;
  } else {
    green_time = time + led_delay/ms_delay; 
  }
  }*/

void hexbright::set_led_state(byte led, byte state, int updates) {
#ifdef DEBUG
  if(DEBUG==DEBUG_BUTTON)
    Serial.println("activate led");
#endif DEBUG
  if(state == LED_OFF) {
  // set the COLOR_time to 0.
    updates = -led_delay/ms_delay;
  }
  if(state == LED_OFF) {
    _set_led(led,LOW);
  } else {
    _set_led(led,HIGH);
  }
  if(led==RLED) {
    red_time = updates + led_delay/ms_delay;
  } else {
    green_time = updates + led_delay/ms_delay; 
  }
}


byte hexbright::get_led_state(byte led) {
  //returns true if the LED is on
  int time;
  if(led==GLED) {
    time = green_time;
  } else {
    time = red_time;
  }
  
  if(time==0) {
    return LED_OFF;
  } else if (time < led_delay/ms_delay) {
    return LED_WAIT;
  } else {
    return LED_ON;
  }
}

 void hexbright::_set_led(byte led, byte state) {
  // this state is HIGH/LOW
  if(led == RLED) { // DPIN_RLED_SW
    if (!released) {
#ifdef DEBUG
      if(DEBUG==DEBUG_BUTTON)
        Serial.println("can't set red led, switch is down");
#endif
      return; 
    }
    if(state == HIGH) {
#ifdef DEBUG
      if(DEBUG==DEBUG_BUTTON)
        Serial.println("Red LED on");
#endif
      digitalWrite(DPIN_RLED_SW, state);
      pinMode(DPIN_RLED_SW, OUTPUT);
    } else {
#ifdef DEBUG
      if(DEBUG==DEBUG_BUTTON)
        Serial.println("Red LED off");
#endif
      pinMode(DPIN_RLED_SW, state);
      digitalWrite(DPIN_RLED_SW, LOW);
    }
  } else { // DPIN_GLED
#ifdef DEBUG
    if(DEBUG==DEBUG_BUTTON)
      if(state==HIGH)
        Serial.println("Green LED on");
      else
        Serial.println("Green LED off");
#endif
    digitalWrite(DPIN_GLED, state);
  }
}

void hexbright::adjust_leds() {
  // turn off led if it's expired
#ifdef DEBUG
  if(DEBUG==DEBUG_BUTTON) {
    if(green_time>0) {
      Serial.print("green countdown: ");
      Serial.println(green_time);
    }
    if(red_time>0) {
      Serial.print("red countdown: ");
      Serial.println(red_time);
    }
  }
#endif
  if(red_time==100/ms_delay){
      _set_led(RLED, LOW);
  } 
  if(red_time!=0) {
    red_time--;
  }
  if(green_time==100/ms_delay){
      _set_led(GLED, LOW);
  }
  if(green_time!=0) {
    green_time--;
  }
}

/////////////////////BUTTON////////////////////

int time_held = 0;

boolean hexbright::button_released() {
  return time_held && released;
}

int hexbright::button_held() {
  return time_held;// && !red_time; 
}

void hexbright::read_button() {
  if(red_time >= 100/ms_delay) { // led is still on, wait
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
      Serial.println(time_held);
#endif
    time_held = 0; 
  } else {
    released = true;
  }
}


///////////////////////////////////////////////
////////////////ACCELEROMETER//////////////////
///////////////////////////////////////////////

// return degrees of movement
//MOVE_TYPE, value returned from a successful detect_movement
#define ACCEL_NONE   0 // nothing
#define ACCEL_TWIST  1 // return degrees - light axis remains constant
#define ACCEL_TURN   2 // return degrees - light axis changes
#define ACCEL_DROP   3 // return change of velocity - period of no acceleration before impact?
#define ACCEL_TAP    4 // return change of velocity - acceleration before impact

boolean using_accelerometer = false;


#define DPIN_ACC_INT 3

#define ACC_ADDRESS             0x4C
#define ACC_REG_XOUT            0
#define ACC_REG_YOUT            1
#define ACC_REG_ZOUT            2
#define ACC_REG_TILT            3
#define ACC_REG_INTS            6
#define ACC_REG_MODE            7


void hexbright::read_accelerometer() {
  
  byte tapped = 0, shaked = 0;
  if (!digitalRead(DPIN_ACC_INT))
  {
    Wire.beginTransmission(ACC_ADDRESS);
    Wire.write(ACC_REG_TILT);
    Wire.endTransmission(false);       // End, but do not stop!
    Wire.requestFrom(ACC_ADDRESS, 1);  // This one stops.
    byte tilt = Wire.read();
    
    static int i = 0; 
    if(!i) {
      tapped = !!(tilt & 0x20);
      shaked = !!(tilt & 0x80);
      Serial.println(tilt);  
      if (tapped) Serial.println("Tap!");
      if (shaked) Serial.println("Shake!");
      
      i=10;
    }
    i--;
  } 
}

void hexbright::enable_accelerometer() {
  pinMode(DPIN_ACC_INT,  INPUT);
  digitalWrite(DPIN_ACC_INT,  HIGH);
}

void hexbright::disable_accelerometer() {
  
}

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
        set_led_state(_color, LED_ON, 400/ms_delay); 
      } else {
        set_led_state(_color, LED_ON, 120/ms_delay);
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
    set_led_state(flip_color(_color), LED_ON, 500/ms_delay);
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
