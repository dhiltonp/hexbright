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

#define LOOP_DELAY 5

///////////////////////////////////////////////
/////////////BUTTON & LED CONTROL//////////////
///////////////////////////////////////////////
// because the red LED and button share the same pin,
// we need to share some logic, hence the merged section.
// the switch cannot receive input while the red led is on.

int red_time = 0;
boolean released = false; //button starts down

// How long LED should remain off after being on?  We'll 
// report being on for this duration, to make printing easier.
#define LED_WAIT 100/LOOP_DELAY

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
    red_time = time + LED_WAIT;
  } else {
    green_time = time + LED_WAIT; 
  }
}

int get_led(int led) {
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
      digitalWrite(DPIN_RLED_SW, state);
      pinMode(DPIN_RLED_SW, OUTPUT);
    } else {
      pinMode(DPIN_RLED_SW, state);
      digitalWrite(DPIN_RLED_SW, LOW);
    }
  } else { // DPIN_GLED
    digitalWrite(led, state);
  }
}

void adjust_leds() {
  // turn off led if it's expired
  switch (red_time) {
    case (0):
      break;
    case (LED_WAIT):
      Serial.println("_set_led(DPIN_RLED_SW, LOW);");
      _set_led(DPIN_RLED_SW, LOW);
    default:
      red_time--;
  } 
  switch (green_time) {
    case (0):
      break;
    case (LED_WAIT):
      _set_led(DPIN_GLED, LOW);
    default:
      green_time--;
  }
}

/////////////////////BUTTON////////////////////

int time_held = 0;

boolean button_released() {
  return time_held && released;
}

int button_held() {
  return time_held;// && !red_time; 
}

void read_button() {
  if(red_time >= LED_WAIT) { // led is still on, wait
    return;
  }
  byte button_on = digitalRead(DPIN_RLED_SW);
  if(button_on) {
    time_held++; 
    released = false;
  } else if (released && time_held) { // we've given a chance for the button press to be read, reset time_held
    time_held = 0; 
  } else {
    released = true;
  }
}


///////////////////////////////////////////////
//////////////////UTILITIES////////////////////
///////////////////////////////////////////////

int _number = 0;
int _color = DPIN_GLED;

boolean printing_number() {
  return _number || waiting(); 
}

void update_number() {
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
    if(!waiting()) {
      if(_number==1) { // minimum delay between printing numbers
        wait(2500/LOOP_DELAY); 
      } else {
        wait(300/LOOP_DELAY); 
      }
      set_led(_color, 120/LOOP_DELAY);
      _number--;
      if(_number && !(_number%10)) { // next digit?
        wait(600/LOOP_DELAY);
        _color = flip_color(_color);
        _number = _number/10;
      }
    }
  } 
}

int flip_color(int color) {
  if (color == DPIN_GLED) {
    return DPIN_RLED_SW; 
  } else {
    return DPIN_GLED;
  }
}

void print_number(int number) {
  // reverse number (so it prints from left to right)
  boolean negative = false;
  if(number<0) {
    number = 0-number;
    negative = true; 
  }
  _color = DPIN_GLED;
  while(number>0) {
    _number = _number * 10 + (number%10); 
    number = number/10;
    _color = flip_color(_color);
  }
  if(negative) {
    set_led(flip_color(_color), 500/LOOP_DELAY); 
  }
}


int _wait_time = 0;
void wait(int time) {
  _wait_time = time;
}

void update_wait() {
  if(_wait_time) {
    _wait_time--;
  } 
}

boolean waiting() {
  return _wait_time;
}


///////////////////////////////////////////////
////////////////TEMPERATURE////////////////////
///////////////////////////////////////////////

int temperature = 0;
void read_temperature() {
  // do not call this directly.  Call get_temperature()
  // read temperature setting
  // device data sheet: http://ww1.microchip.com/downloads/en/devicedoc/21942a.pdf
  
  temperature = analogRead(APIN_TEMP);
}

int get_celsius(int temp) {
  // 0C ice water bath for 20 minutesls: 153.
  // 40C water bath for 20 minutes (measured by medical thermometer): 275
  // intersection with 0: 50 = (40C-0C)/(275-153)*153
  // readings obtained with DEBUG_TEMP
  return temp * (40-0)/(275-153) - 50;
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
//#define LOOP_DELAY 5 // placed at top of file, because adjust_number 
// and adjust_led need it

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
  
  // turn on light (lowest setting) so we don't turn off
  pinMode(DPIN_PWR, OUTPUT);
  digitalWrite(DPIN_PWR, HIGH);
  digitalWrite(DPIN_DRV_MODE, LOW);
  analogWrite(DPIN_DRV_EN, 1);
    
  last_time = millis();
}

void loop() {
  unsigned long time = millis();
  if(time-last_time>LOOP_DELAY) {
    // power saving modes described here: http://www.atmel.com/Images/2545s.pdf
    //run overheat protection, time display, track battery usage
    read_button();
    read_temperature(); // takes about .2 ms to execute (fairly long, relative to the other steps)
    update_wait();
    update_number();
    adjust_leds();

    if(!printing_number()) {
      print_number(get_temperature());
//      print_number(get_fahrenheit(get_temperature()));
    }
    
    last_time = time;
  }
}

