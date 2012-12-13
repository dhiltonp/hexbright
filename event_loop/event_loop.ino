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
#define DPIN_RLED_SW 2
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
#define DEBUG_BUTTON 4 // button presses


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
  int new_temperature = get_temperature();
  
  safe_light_level = safe_light_level+(OVERHEAT_TEMPERATURE-new_temperature);
  // min, max levels...
  safe_light_level = safe_light_level > MAX_LIGHT_LEVEL ? MAX_LIGHT_LEVEL : safe_light_level;
  safe_light_level = safe_light_level < 0 ? 0 : safe_light_level;
#ifdef DEBUG
  if(DEBUG==DEBUG_TEMP) {
    Serial.print("Estimated safe light level: ");
    Serial.println(safe_light_level);
    Serial.print("Current temperature: ");
    Serial.println(new_temperature);
  }
#endif

  // if safe_light_level has changed, guarantee a light adjustment:
  if(safe_light_level < MAX_LIGHT_LEVEL) {
#ifdef DEBUG
  DEBUG==DEBUG_LIGHT || DEBUG==DEBUG_TEMP ? Serial.println("safe light level not maximum, adjust"):0;
#endif
    change_done  = min(change_done , change_duration);
  }
}

///////////////////////////////////////////////
////////////////BUTTON CNTL////////////////////
///////////////////////////////////////////////

int time_held = 0;
boolean released;

boolean button_released() {
  return time_held && released;
}

int button_held() {
  return time_held; 
}

void read_button() {
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

int get_temperature() {
  return temperature;
}


///////////////////////////////////////////////
////////////////MAIN EXECUTION/////////////////
///////////////////////////////////////////////

#define LOOP_DELAY 10

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
#endif
  
  last_time = millis();
  set_light_adjust(0,1000,1000);
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
    read_button();
    read_temperature();
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

void control_action() {
  static int brightness = 0;
  if(button_released()) {
    if(button_held()<30) {
      mode = CYCLE_MODE;
      brightness = (brightness + 250) % 1000;
      set_light_adjust(CURRENT_LEVEL, brightness, 30);
    } else if (button_held() < 100) {
      mode = BLINKY_MODE;
    }
  }
  if(mode == BLINKY_MODE) {
    static int i = 0;
    if(!i) {
      set_light_adjust(MAX_LOW_LEVEL,0,5);
      i=20;
    }
    i--;
  }
  if(button_held()>100) {
    mode = OFF_MODE;
    brightness = 0;
    set_light_adjust(0,0,1);
  }
}
