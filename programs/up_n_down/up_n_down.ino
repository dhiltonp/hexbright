/*
Copyright (c) 2014, "Whitney Battestilli" <whitney@battestilli.net>
Portions Copyright (c) 2014, "Matthew Sargent" <matthew.c.sargent@gmail.com>
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
*/

#include <click_counter.h>
#include <print_power.h>

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>
#include <EEPROM.h>

#if (DEBUG==DEBUG_PROGRAM)
#define DBG(a) a
#else
#define DBG(a)
#endif

#define EEPROM_LOCKED 0
#define EEPROM_NIGHTLIGHT_BRIGHTNESS 1
#define EEPROM_SIGNATURE_LOC 2 // through 12

// EEPROM signature used to verify EEPROM has been initialize
static const char* EEPROM_Signature = "UP_N_DOWN";

// Modes
#define MODE_OFF 0
#define MODE_LEVEL 1
#define MODE_BLINK 2
#define MODE_NIGHTLIGHT 3
#define MODE_SOS 4
#define MODE_LOCKED 5

 // Defaults
static const int glow_mode_time = 3000;
static const int click = 350; // maximum time for a "short" click
static const int nightlight_timeout = 5000; // timeout before nightlight powers down after any movement
static const unsigned char nightlight_sensitivity = 20; // measured in 100's of a G.
static const byte sos_pattern[] = {0xA8, 0xEE, 0xE2, 0xA0};
static const int singleMorseBeat = 150;

// State
static unsigned long treg1=0; 

static int bitreg=0;
const unsigned GLOW_MODE=0;
const unsigned GLOW_MODE_JUST_CHANGED=1;
const unsigned QUICKSTROBE=2;

static char mode = MODE_OFF;
static char new_mode = MODE_OFF;  
static unsigned tailflashesLeft = 0;

static word nightlight_brightness;

const word blink_freq_map[] = {70, 650, 10000}; // in ms
static word blink_frequency; // in ms;

static byte locked;
static int sos_cursor = 0;
unsigned long time;
hexbright hb;

int adjustLED() {
  if(hb.button_pressed() && hb.button_pressed_time()>click) {
    double d = hb.difference_from_down(); // (0,1)
    int di = (int)(d*400.0); // (0,400)
    int i = (di)/10; // (0,40)
    i *= 50; // (0,2000)
    i = i>1000 ? 1000 : i;
    i = i<=0 ? 1 : i;

    hb.set_light(CURRENT_LEVEL, i, 100);

    return i;
  }
  return -1;
}

//Write the bytes contained in the parameter array to the location in 
//EEPROM starting at startAddr. Write numBytes number of bytes.
void writebytesEEPROM(int startAddr, const byte* array, int numBytes) {
  int i;
  for (i = 0; i < numBytes; i++) {
    updateEEPROM(startAddr+i,array[i]);
  }
}

byte updateEEPROM(word location, byte value) {
  byte c = EEPROM.read(location);
  if(c!=value) {
    DBG(Serial.print("Write to EEPROM:"); Serial.print(value); Serial.print("to loc: "); Serial.println(location));
    EEPROM.write(location, value);
  }
  return value;
}

//Check the EEPROM to see if the signature has been written, if found, return true
boolean checkEEPROMInited(word location) {
  //read starting at location and look for the signature.
  int i = 0;
  for (i = 0; i < strlen(EEPROM_Signature); i++) {
    if (EEPROM_Signature[i] != EEPROM.read(location+i))
      return false; //EEPROM does not match Signature, so EEPROM not initialized
  }
  return true; //The signature was found, so the EEPROM is initialized...
}

void setup() {
  // We just powered on!  That means either we got plugged
  // into USB, or the user is pressing the power button.
  hb = hexbright();
  hb.init_hardware();

  //The following code detects if the EEPROM has been initialized
  //If it has not, initialize it.
  if(!checkEEPROMInited(EEPROM_SIGNATURE_LOC)) {

    updateEEPROM(EEPROM_LOCKED,0);  
    updateEEPROM(EEPROM_NIGHTLIGHT_BRIGHTNESS, 100);

    //now store the signature so we know the EEPROM has been initialized
    int length = strlen(EEPROM_Signature) + 1; //this will cause the nul to be written too.
    writebytesEEPROM(EEPROM_SIGNATURE_LOC,(const byte*)EEPROM_Signature, length);
  }

  //DBG(Serial.println("Reading defaults from EEPROM"));
  locked = EEPROM.read(EEPROM_LOCKED);
  //DBG(Serial.print("Locked: "); Serial.println(locked));
  nightlight_brightness = EEPROM.read(EEPROM_NIGHTLIGHT_BRIGHTNESS)*4;
  nightlight_brightness = nightlight_brightness==0 ? 1000 : nightlight_brightness;
  //DBG(Serial.print("Nightlight Brightness: "); Serial.println(nightlight_brightness));
  
  DBG(Serial.println("Powered up!"));

  config_click_count(click);
} 

void loop() {
  time = millis();

  hb.update();
  
#ifdef PRINTING_NUMBER:
  if(!hb.printing_number()) 
#endif
  {
    // Charging
    print_charge(GLED);

    // Low battery
    if(mode != MODE_OFF && hb.low_voltage_state())
      if(hb.get_led_state(RLED)==LED_OFF) 
	hb.set_led(RLED,50,1000,1);
  }

  // Get the click count
  new_mode = click_count();

 if(new_mode>=MODE_OFF) {
    DBG(Serial.print("New mode: "); 
    Serial.println((int)new_mode));

    //If the light is on now, any new mode request is converted to a MODE_OFF.
    //While the light is on, you can not switch it into another mode, it just goes off. 
    if(mode!=MODE_OFF) {
      DBG(Serial.println("Forcing MODE_OFF"));
      new_mode=MODE_OFF;
    
    //The following keeps the light locked and off until the light is unlocked.
    //Flash the tailcap 2 times to indicate the light is locked.
    } else if (locked && new_mode!=MODE_LOCKED){
      new_mode=MODE_OFF;
      DBG(Serial.println("Locked, flash tailcap 2 times."));
      tailflashesLeft = 2;
    }
  }

  // Do the actual mode change
  if(new_mode>=MODE_OFF && new_mode!=mode) {
    double d;
    int i;
    //Clear tailflashesLeft if mode is not OFF. 
    //If the user switched to a new mode while the tail was flashing, it may not yet be zeroed
    if (new_mode != MODE_OFF)
      tailflashesLeft = 0;

    // we're changing mode
    switch(new_mode) {
    case MODE_LOCKED:
      locked=!locked;
      updateEEPROM(EEPROM_LOCKED,locked);
      new_mode=MODE_OFF;
      //Setup the tail light to flash...
      tailflashesLeft = 3;
      /* fall through */
    case MODE_OFF:
      //While in Glow mode or while flashing the tail, do not shutdown the uProcessor, 
      //this is done by using a level of 0 instead of the OFF_LEVEL
      DBG(BIT_CHECK(bitreg,GLOW_MODE)||tailflashesLeft>0?Serial.println("Stay alive"):Serial.println("Shut down"));
      if (BIT_CHECK(bitreg,GLOW_MODE)||tailflashesLeft>0) {
        hb.set_light(CURRENT_LEVEL, 0, NOW);
      } else {
	hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
	DBG(Serial.println("Shut down"));
      }

      if(mode==MODE_NIGHTLIGHT) {
	//DBG(Serial.print("Nightlight Brightness Saved: "); Serial.println(nightlight_brightness));
	updateEEPROM(EEPROM_NIGHTLIGHT_BRIGHTNESS, nightlight_brightness/4);
      }
      break;
    case MODE_LEVEL:
      d = hb.difference_from_down();
      i = MAX_LEVEL;
      if(d <= 0.40) {
	if(d <= 0.10)
	  i = 1;
	else
	  i = MAX_LOW_LEVEL;
      }
      hb.set_light(CURRENT_LEVEL, i, NOW); 
      break;
    case MODE_NIGHTLIGHT:
      DBG(Serial.print("Nightlight Brightness: "); Serial.println(nightlight_brightness));
#ifdef PRINTING_NUMBER:
      if(!hb.printing_number())
#endif
	hb.set_led(RLED, 100, 0, 1);
      break;
    case MODE_SOS:
      //Entering SOS mode, so set the cursor at the beginning of the pattern
      sos_cursor = 0;
      
      //Give a light change command with time of NOW so it finishes right away 
      //making the SOS pattern start without much delay.
      hb.set_light(0, 0, NOW); 
      break;
    case MODE_BLINK:
      d = hb.difference_from_down();
      blink_frequency = blink_freq_map[0];
      if(d <= 0.40) {
	if(d <= 0.10)
	  blink_frequency = blink_freq_map[2];
	else
	  blink_frequency = blink_freq_map[1];
      }
      hb.set_light(MAX_LEVEL, 0, 20);
      break;
    }
    mode=new_mode;
  }

  // Check for mode and do in-mode activities
  switch(mode) {
  case MODE_OFF:
    if (tailflashesLeft > 0) { //flashing tail
      if (hb.get_led_state(locked?RLED:GLED)==LED_OFF) {
        tailflashesLeft--;
        hb.set_led(locked?RLED:GLED, 300, 300, 255);
      }
    } 
    // glow mode
    else if(BIT_CHECK(bitreg,GLOW_MODE)) {
      hb.set_led(GLED, 100, 100, 64);
      hb.set_light(CURRENT_LEVEL, 0, NOW);
    } else if(BIT_CHECK(bitreg,GLOW_MODE_JUST_CHANGED)) {
      hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    }

    // holding the button
    if(hb.button_pressed() && !locked) {
	double d = hb.difference_from_down();
	if(BIT_CHECK(bitreg,QUICKSTROBE) || (hb.button_pressed_time() > click && d > 0.10 )) {
	  BIT_SET(bitreg,QUICKSTROBE);
	  if(treg1+blink_freq_map[0] < time) { 
	    treg1 = time; 
	    hb.set_light(MAX_LEVEL, 0, 20); 
	  }
	}
	if(hb.button_pressed_time() >= glow_mode_time && d <= 0.1 && 
	   !BIT_CHECK(bitreg,GLOW_MODE_JUST_CHANGED) && !BIT_CHECK(bitreg,QUICKSTROBE) ) {
	  BIT_TOGGLE(bitreg,GLOW_MODE);
	  BIT_SET(bitreg,GLOW_MODE_JUST_CHANGED);
	} 
	
    }
    if(hb.button_just_released()) {
      BIT_CLEAR(bitreg,GLOW_MODE_JUST_CHANGED);
      BIT_CLEAR(bitreg,QUICKSTROBE);
    }
    break;
  case MODE_LEVEL:
    adjustLED();
    break;
  case MODE_NIGHTLIGHT: {
    if(!hb.low_voltage_state())
      hb.set_led(RLED, 100, 0, 1);
    if(hb.moved(nightlight_sensitivity)) {
      //Serial.println("Nightlight Moved");
      treg1 = time;
      hb.set_light(CURRENT_LEVEL, nightlight_brightness, 1000);
    } else if(time > treg1 + nightlight_timeout) {
      hb.set_light(CURRENT_LEVEL, 0, 1000);
   }
    int i = adjustLED();
    if(i>0) {
      //DBG(Serial.print("Nightlight Brightness: "); Serial.println(i));
      nightlight_brightness = i;
    }
    break; }
  case MODE_BLINK:
    if(hb.button_pressed()) {
      if( hb.button_pressed_time()> click) {
	double d = hb.difference_from_down();
	if(d>=0 && d<=0.99) {
	  if(d>=0.25) {
	    d = d>0.5 ? 0.5 : d;
	    blink_frequency = blink_freq_map[0] + (word)((blink_freq_map[1] - blink_freq_map[0]) * 4 * (0.5-d));
	  } else {
	    blink_frequency = blink_freq_map[1] + (word)((blink_freq_map[2] - blink_freq_map[1]) * 4 * (0.25-d));
	  }
	  //DBG(Serial.print("Blink Freq: "); Serial.println(blink_frequency));
	}
      }
    }
    if(treg1+blink_frequency < time) { 
      treg1 = time;
      hb.set_light(MAX_LEVEL, 0, 20); 
    }
    break;
  case MODE_SOS:
    //Send the international distress signal.  
    //... --- ...   ... --- ...
    //Dit  150 = 1 beat
    //Dah  450 = 3 beats
    //Inter dit/dah  = 1 beat
    //Inter character = 3 beats = <ic>
    //Inter word  = 7 beats = <iw>

    //From left to right, the pattern is encoded with bits, each bit being
    //a singleMorseBeat long (except the last one, as in:
    //<-S-> <inter-char> <----O----> <inter-char> <-S-> <-inter word->
    //10101     000      11101110111      000     10101  0xxx
    //
    //10101000 11101110 11100010 1010xxxx
    //    0xA8     0xEE     0xE2     0xA0     

    //At the end of each completed light change, move to the next bit.
    //In this case the light change will be from ON to ON or from Off to Off, so
    //that is how we get our flashing. This should happen every singleMorseBeat, except 
    //on the last bit where we cause the delay to be longer (x7) to mark the between word space
    if(hb.light_change_remaining()==0){
      int whichByte = sos_cursor/8; //0/8 = 0 1/8=0...
      int whichBit = 7 - sos_cursor%8; //7 - 0%8 = 7; 7-30%8 = 6
      int onOffAmount = BIT_CHECK(sos_pattern[whichByte],whichBit)?1000:0;

      //if this is the last bit, then make the delay an interword amount (x7)
      int onOrOffTime = sos_cursor<27?singleMorseBeat:singleMorseBeat*7; 
      
      hb.set_light(onOffAmount,onOffAmount, onOrOffTime);
      
      DBG(Serial.print("SOS Data; byte#/bit#: "); Serial.print(whichByte); Serial.print("/"); Serial.println(whichBit));
      DBG(Serial.print("SOS Data; onOffAmount#: "); Serial.println(onOffAmount));
      DBG(Serial.print("SOS Data; onOrOffTime: "); Serial.println(onOrOffTime));
      sos_cursor++;

      if (sos_cursor>27 || sos_cursor<0) {
        sos_cursor = 0;
      }
    }
    break;
  }  

}

