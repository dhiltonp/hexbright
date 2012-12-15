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



#include <math.h>
#include <Wire.h>


// Pin assignments
#define DPIN_RLED_SW 2 // both red led and switch.  pinMode OUTPUT = led, pinMode INPUT = switch
#define DPIN_GLED 5
#define DPIN_PWR 8
#define DPIN_DRV_MODE 9
#define DPIN_DRV_EN 10
#define APIN_TEMP 0
#define APIN_CHARGE 3

//#define DEBUG 4
#define DEBUG_LOOP 1 // main loop
#define DEBUG_LIGHT 2 // Light control
#define DEBUG_TEMP 3  // temperature safety
#define DEBUG_BUTTON 4 // button presses/rear led


///////////////////////////////////////////////
////////////////LIGHT CONTROL//////////////////
///////////////////////////////////////////////

// Light level must be sufficiently precise for quality low-light brightness and accurate power adjustment at high brightness.
// light level should be converted to logarithmic, square root or cube root values (from lumens), so as to be perceptually linear...
// http://www.candlepowerforums.com/vb/newreply.php?p=3889844
// This is handled inside of set_light_level.

#define MAX_LIGHT_LEVEL 1000
#define MAX_LOW_LEVEL 385
#define CURRENT_LEVEL -1

#ifdef DEBUG
#define OVERHEAT_TEMPERATURE 265 // something low, to verify smoothing is working right
#define OVERHEAT_TEMPERATURE 300 // something low, to verify smoothing is working right
#else
#define OVERHEAT_TEMPERATURE 330 // 340 in original code, 330 to give us some overhead for the adjustment algorithm
#endif


int start_light_level = 0;
int end_light_level = 0;
int change_duration = 0;
int change_done  = 0;

int safe_light_level = MAX_LIGHT_LEVEL;

void set_light_adjust(int _start_light_level, int light_level, int duration) {
// duration ranges from 1-MAXINT
// light_level can be from 0-1000
  if(_start_light_level == CURRENT_LEVEL) {
    start_light_level = get_light_level();
    end_light_level = light_level;
  } else {
    start_light_level = _start_light_level;
    end_light_level = light_level;
  }

  change_duration = duration;
  change_done = 0;
#ifdef DEBUG
  if (DEBUG == DEBUG_LIGHT) {
    Serial.print("Light adjust requested, start level:");
    Serial.println(start_light_level);
  }
#endif

}


void _set_light(int mode, int brightness) {
#ifdef DEBUG
  if (DEBUG == DEBUG_LIGHT) {
    Serial.print("Power/MODE: ");
    Serial.print(brightness);
    Serial.print("/");
    Serial.println(mode);
  }
#endif
  pinMode(DPIN_PWR, OUTPUT);
  digitalWrite(DPIN_PWR, HIGH);
  digitalWrite(DPIN_DRV_MODE, mode);
  analogWrite(DPIN_DRV_EN, brightness);
}

boolean high_output_mode = false;

void adjust_light() {
  // sets actual light level, altering value to be perceptually linear, based on steven's area brightness (cube root)
  // don't use this directly, use set_light_adjust(level, LINEAR, 0);

  if(change_done<=change_duration) {
    int light_level = get_light_level();
    
    if(!light_level) {
#ifdef DEBUG
      if(DEBUG==DEBUG_LIGHT) {
        Serial.println("Light level low, turning off");
      }
#endif
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, LOW);
      digitalWrite(DPIN_DRV_MODE, LOW);
      digitalWrite(DPIN_DRV_EN, LOW);
    } else {
      // brightness has some issues with linearity when switching between power modes, but works ok
      float brightness = pow(light_level, 3)/227000+3;
#ifdef DEBUG
      if(DEBUG==DEBUG_LIGHT) {
        Serial.print("Perceptual brightness: ");
        Serial.print(light_level);
        Serial.print("/");
        Serial.println(MAX_LIGHT_LEVEL);
      }
#endif
      // LOW 255 approximately equals HIGH 48/49.  There is a color change.  
      // When switching from 254 or less to HIGH:48/49, I get a more noticeable glitch.
      // values < 4 do not provide any light.  (that's the +3 in the brightness calculation)
      // I don't know about relative power draw.
      if(brightness<255) {
        _set_light(LOW, brightness);
      } else { // bright mode
        if (false && !high_output_mode) {
          _set_light(LOW, 255);
          high_output_mode = true;
        }
        brightness = (brightness - 255)/20+48;
        _set_light(HIGH, brightness);
      }
    }
    change_done++;
  }
}

int get_light_level() {
  // return current brightness level (soft)
  int brightness = (end_light_level-start_light_level)*((float)change_done/change_duration) +start_light_level;

  if(brightness>safe_light_level)
     return safe_light_level;
  return brightness;
}

  // If the starting temp is much higher than max_temp, it may be a long time before you can turn the light on.
  // this should only happen if: your ambient temperature is higher than max_temp, or you adjust max_temp while it's still hot.
  // Here's an example: ambient temperature is > 
void overheat_protection() {
  int temperature = get_temperature();
  
  safe_light_level = safe_light_level+(OVERHEAT_TEMPERATURE-temperature);
  // min, max levels...
  safe_light_level = safe_light_level > MAX_LIGHT_LEVEL ? MAX_LIGHT_LEVEL : safe_light_level;
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
      Serial.print(get_celsius(printed_temperature));
      Serial.print(") (fahrenheit: ");
      Serial.print(get_fahrenheit(printed_temperature));
      Serial.println(")");
    }
  }
#endif

  // if safe_light_level has changed, guarantee a light adjustment:
  if(safe_light_level < MAX_LIGHT_LEVEL) {
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

int red_time = 0;
boolean released = false; //button starts down

//////////////////////LED//////////////////////

int green_time = 0;


void set_led(int led, int time) {
// led = DPIN_GLED or DPIN_RLED_SW,
// time = cycles before led is turned off (0=now)
  if(time==0) {
    _set_led(led,LOW);  
  } else {
    _set_led(led,HIGH);  
  }
  if(led==DPIN_RLED_SW) {
    red_time = time; 
  } else {
    green_time = time; 
  }
}

boolean get_led(int led) {
  //returns true if the LED is on
  if(led==DPIN_RLED_SW) {
    return red_time;
  } else {
    return green_time;
  }
}

void _set_led(int led, int state) {
// Avoid calling directly, it's easy to mess up.  Call led_pulse, if possible.
  if(led == DPIN_RLED_SW) {
    if (!released) {
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
    digitalWrite(led, state);
  }
}

void adjust_leds() {
  // turn off led if it's expired
  if(red_time == 1) {
    _set_led(DPIN_RLED_SW, LOW);
    red_time--; 
  } else if(red_time == 0) {
    // nothing...
  } else {
    red_time--; 
  }
  if(green_time == 1) {
    _set_led(DPIN_GLED, LOW);
    green_time--;
  } else if (green_time == 0) {
    // nothing 
  } else {
    green_time--;
  }
}

/////////////////////BUTTON////////////////////

int time_held = 1; // button starts down

boolean button_released() {
  return time_held && released;
}

int button_held() {
  return time_held; 
}

void read_button() {
  if(red_time > 0) { // led is still on, wait
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
////////////////TEMPERATURE////////////////////
///////////////////////////////////////////////

int temperature = 0;
void read_temperature() {
  // do not call this directly.  Call get_temperature()
  // read temperature setting
  temperature = analogRead(APIN_TEMP);
}

int get_celsius(int temp) {
  // 0C ice water bath for 30 minutes: 153.
  // 35C water bath for 30 minutes (measured by medical thermometer): 255
  // intersection with 0: 52.5 = (35C-0C)/(255-153)*153
  return temp * (35-0)/(255-153) - 53; 
}

int get_fahrenheit(int temp) {
  return get_celsius(temp) * 18 / 10 + 32;
}

int get_temperature() {
  return temperature;
}


///////////////////////////////////////////////
////////////////MAIN EXECUTION/////////////////
///////////////////////////////////////////////

// the point of LOOP_DELAY is to provide regular update speeds,
// so if code takes longer to execute from one run to the next,
// the actual interface doesn't change (button click duration,
// brightness changes). Set this from 1-30. very low is generally
// fine (or great), BUT if you do any printing, the actual delay
// will be greater than the value you set.
// Consider using 100/LOOP_DELAY, where 100 is 100 milliseconds
// btw, 100/LOOP_DELAY should be evaluated at compile time.
#define LOOP_DELAY 30

unsigned long last_time;

void setup()
{
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
    set_light_adjust(0,1000,1000);
  } else if (DEBUG==DEBUG_TEMP) {
    // turn up the heat!
    set_light_adjust(0,1000,1);
  } else if (DEBUG==DEBUG_LOOP) {
    // note the use of TIME_MS/LOOP_DELAY.
    set_light_adjust(0,1000,2500/LOOP_DELAY);
  }
#endif
  
  last_time = millis();
  // setup initial light state
}

void loop() {
  unsigned long time = millis();

  // loop 200? 60? times per second?
  // The point is, we want light adjustments to be constant regardless of how much processing is going on.
  if(time-last_time >= LOOP_DELAY) {
#ifdef DEBUG
    static int i=0;
    static float avg_loop_time = 0;
    avg_loop_time = (avg_loop_time*29 + time-last_time)/30;
    if(DEBUG==DEBUG_LOOP && !i) {
      Serial.print("Average loop time: ");
      Serial.println(avg_loop_time);
    }
    if(avg_loop_time>LOOP_DELAY+1 && !i) {
      // This may be caused by too much processing for our LOOP_DELAY, or by too many print statements (each one takes a few ms)
      Serial.print("WARNING: loop time: ");
      Serial.println(avg_loop_time);
    }
    if (!i)
      i=1000/LOOP_DELAY; // display loop output every second
    else
      i--;
#endif
    // power saving modes described here: http://www.atmel.com/Images/2545s.pdf
    //run overheat protection, time display, track battery usage
    adjust_light(); // change light levels as requested
    adjust_leds();
    read_button();
    read_temperature(); // takes about .2 ms to execute (fairly long, relative to the other steps)
    overheat_protection();    
    
    // take action based on mode/input
    control_action();

    // update time
    last_time = time;
  }
}


/*
void control_action() {
  static int brightness = 100;
  if(button_released() && button_held()<30) {
    brightness = (CURRENT_LEVEL, brightness % 1000) + 100;
    set_light_adjust(brightness, 25);
  }
  if(button_held()>30 && button_released()) {
    set_light_adjust(0,0,1);
    brightness = 100;
  }
} */

#define OFF_MODE 0
#define BLINKY_MODE 1
#define CYCLE_MODE 2

int mode = 0;

// BLINKY_MODE has some issues when not connected to USB.  It detects a poweroff somehow.
// very quick releases of buttons aren't working right, not sure why.
void control_action() {
  static int brightness = 0;
  // flash once (100 ms) for every 400 ms period the button has been down
  if((button_held()-1)%(400/LOOP_DELAY)==0) {
    set_led(DPIN_GLED, 100/LOOP_DELAY);
  }
  if(button_released()) {
    if(button_held()<2) {
      // ignore, could be a bounce
    } else if(button_held()<250/LOOP_DELAY) {
      mode = CYCLE_MODE;
      brightness = (brightness + 250) % 1250;
      set_light_adjust(CURRENT_LEVEL, brightness, 150/LOOP_DELAY);
    } else if (button_held() < 500/LOOP_DELAY) {
      mode = BLINKY_MODE;
    }
  }
  if(mode == BLINKY_MODE) {
    static int i = 0;
    if(!i) {
      set_light_adjust(MAX_LOW_LEVEL,1,120/LOOP_DELAY);
      i=20;
    }
    i--;
  }
  if(button_held()>500/LOOP_DELAY) {
    mode = OFF_MODE;
    brightness = 0;
    set_light_adjust(0,0,1);
  }
}

