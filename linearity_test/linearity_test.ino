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

//#define DEBUG
#define LOOP_DELAY 3


void set_light(int mode, int brightness) {
#ifdef DEBUG
  Serial.print("Power/MODE: ");
  Serial.print(brightness);
  Serial.print("/");
  Serial.println(mode);
#endif
  digitalWrite(DPIN_DRV_MODE, mode);
  analogWrite(DPIN_DRV_EN, brightness);
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

#ifdef DEBUG
  Serial.begin(9600);
  Wire.begin();
  Serial.println("DEBUG MODE ON");
#endif
  
  pinMode(DPIN_PWR, OUTPUT);
  digitalWrite(DPIN_PWR, HIGH);

  
  last_time = millis();
}

void loop() {
  unsigned long time = millis();

  // loop 200? 60? times per second?
  // The point is, we want light adjustments to be constant regardless of how much processing is going on.
  if(time-last_time >= LOOP_DELAY) {
    static int level = LOW;
    static unsigned long i=0;
    static int maxnum = 500;
    static double exponent = 2.5;
    if(level == LOW) {
      set_light(level, pow(i,exponent)*251/pow(maxnum,exponent)+4);
      i++;
      if(i>maxnum) {
        // 7, 225, 30
        // 8, 232, 23
        i=7*maxnum/20;
//        maxnum = 600;
        level = HIGH;
        exponent = 3;
      }
    }
    if(level == HIGH) {
//      set_light(level, pow(i,exponent)*241/pow(maxnum,exponent)+14);
      set_light(level, pow(i,exponent)*225/pow(maxnum,exponent)+30);
      i++;
      if(i>maxnum) {
        i=0;
        level = LOW;
//        maxnum = 600;
        exponent = 2.5;
      }
    }
    
    // update time
    last_time = time;
  }
}

