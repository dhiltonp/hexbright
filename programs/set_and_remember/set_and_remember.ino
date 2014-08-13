/*
Copyright (c) 2013, "Whitney Battestilli" <whitney@battestilli.net>
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

/*
Changes and modifications are Copyright (c) 2014, "Matthew Sargent" <matthew.c.sargent@gmail.com>
 
 This code is a direct modification of the code up_n_down as copyrighted above.
 
 Description:
 
 1 click: Turn light on (from off) at the saved level.
 2 clicks: Blink
 3 clicks: Nightlight mode, movement turns on using nightlight stored level
 4 clicks: SOS using remembered level
 5 clicks: locking, flashes the switch Red or Green LED 3 times; Red entering locked, green exiting it.
 
 Click and Hold (while ON): 
 Set level; point down = lowest, point horizon = highest, save this level.
 In Nightlight Mode; point down lowest, point horizon highest, save this as nightlight level.
 
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
#define EEPROM_STORED_BRIGHTNESS 2
#define EEPROM_SIGNATURE_LOC 3

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

// EEPROM signature used to verify EEPROM has been initialize
static const char* EEPROM_Signature = "Set_and_Remember";

// State
static unsigned long treg1=0; 

static int bitreg=0;
const unsigned BLOCK_TURNING_OFF=0;
const unsigned GLOW_MODE=1;
const unsigned GLOW_MODE_JUST_CHANGED=2;
const unsigned QUICKSTROBE=3;

static char mode = MODE_OFF;
static char new_mode = MODE_OFF;  
static char submode;
static unsigned tailflash = 0;

static word nightlight_brightness;
static word stored_brightness;

const word blink_freq_map[] = { 70, 650, 10000}; // in ms
static word blink_frequency; // in ms;

static byte locked = false;

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
    BIT_SET(bitreg,BLOCK_TURNING_OFF);
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

//Update the EEPROM, but check to make sure we actually need to update it first.
//This saves writing to the EEPROM when it is not necessary (important).
byte updateEEPROM(word location, byte value) {
  byte c = EEPROM.read(location);
  if(c!=value) {
    DBG(Serial.print("Write to EEPROM:"); 
    Serial.print(value); 
    Serial.print("to loc: "); 
    Serial.println(location));
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
    updateEEPROM(EEPROM_STORED_BRIGHTNESS, 150);

    //now store the signature so we know the EEPROM has been initialized
    int length = strlen(EEPROM_Signature) + 1; //this will cause the nul to be written too.
    writebytesEEPROM(EEPROM_SIGNATURE_LOC,(const byte*)EEPROM_Signature, length);
  }

  //Initialize variables using the values stored in the EEPROM  

  //DBG(Serial.println("Reading defaults from EEPROM"));
  locked = EEPROM.read(EEPROM_LOCKED);
  //DBG(Serial.print("Locked: "); Serial.println(locked));
  nightlight_brightness = EEPROM.read(EEPROM_NIGHTLIGHT_BRIGHTNESS)*4;
  nightlight_brightness = nightlight_brightness==0 ? 1000 : nightlight_brightness;
  //DBG(Serial.print("Nightlight Brightness: "); Serial.println(nightlight_brightness));

  //Fetch the stored brightness from EEPROM and use it when turning on.
  stored_brightness = EEPROM.read(EEPROM_STORED_BRIGHTNESS)*4;
  stored_brightness = stored_brightness==0 ? 1000 : stored_brightness; //If zero, default to 1000
  //DBG(Serial.print("Stored Brightness: "); Serial.println(stored_brightness));

  DBG(Serial.println("Powered up!"));

  config_click_count(click);
} 

void loop() {
  const unsigned long time = millis();

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


  /////////////////////////////////////////////////////////////////
  // Check for the requested new mode and do the mode switching activities
  /////////////////////////////////////////////////////////////////
  // Get the click count
  new_mode = click_count();

  if(new_mode>=MODE_OFF) {
    DBG(Serial.print("New mode: "); 
    Serial.println((int)new_mode));

    //The following causes any number of clicks to switch the light OFF when it is currently ON.
    //For instance, while the light is on, you can not switch it into another mode, it just goes off. 
    //Also, this if tests if the light is currently locked, if it is, remain locked and in mode OFF.
    if(mode!=MODE_OFF || (locked && new_mode!=MODE_LOCKED) ) {
      DBG(Serial.println("Forcing MODE_OFF"));
      new_mode=MODE_OFF;
    } 
  }

  // Do the actual mode change
  if(new_mode>=MODE_OFF && new_mode!=mode) { // has the mdoe actually changed?
    double d;

    //Clear tailflash if mode is not OFF. 
    //If the user switched to a new mode while the tail was flashing, it may not yet be zeroed
    if (new_mode != MODE_OFF)
      tailflash = 0;

    // we're changing mode
    switch(new_mode) {

    case MODE_LOCKED:
      locked=!locked;//toggle the locked mode
      updateEEPROM(EEPROM_LOCKED,locked); //remember if we are locked or not.
      new_mode=MODE_OFF; //keep the light off when entering or exiting lock mode.
      //Setup the tail light to flash...
      tailflash = 3;

      /* fall through */
    case MODE_OFF:
      //While in Glow mode or while flashing the tail, do not shutdown the uProcessor, 
      //this is done by using a level of 0 instead of the OFF_LEVEL
      DBG(BIT_CHECK(bitreg,GLOW_MODE)||tailflash>0?Serial.println("Stay alive"):Serial.println("Shut down"));
      if (BIT_CHECK(bitreg,GLOW_MODE)||tailflash>0)
        hb.set_light(CURRENT_LEVEL, 0, NOW);
      else {
        //We are shutting down, so save off the values
        //If we are in nightlight node, save the brightness that should be used when 
        //the light returns on from nightlight mode.
        if(mode==MODE_NIGHTLIGHT) {
          //DBG(Serial.print("Nightlight Brightness Saved: "); Serial.println(nightlight_brightness));
          updateEEPROM(EEPROM_NIGHTLIGHT_BRIGHTNESS, nightlight_brightness/4);
        }

        //Save the stored_brightness in the EEPROM so we can use it next time
        //DBG(Serial.print("Brightness Saved: "); Serial.println(stored_brightness));
        updateEEPROM(EEPROM_STORED_BRIGHTNESS, stored_brightness/4);

        //Shut her down...
        hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
      }
      break;

    case MODE_LEVEL: 
      //just turn on the light to the saved level
      hb.set_light(CURRENT_LEVEL, stored_brightness, NOW); 
      break;

    case MODE_NIGHTLIGHT:
      DBG(Serial.print("Nightlight Brightness: "); 
      Serial.println(nightlight_brightness));
#ifdef PRINTING_NUMBER:
      if(!hb.printing_number())
#endif
        hb.set_led(RLED, 100, 0, 1);
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

  /////////////////////////////////////////////////////////////////
  // Check for mode and do in-mode activities
  /////////////////////////////////////////////////////////////////

  int i;

  switch(mode) {
  case MODE_OFF:
    if (tailflash > 0) { //flashing tail
      if (hb.get_led_state(locked?RLED:GLED)==LED_OFF) {
        tailflash--;
        hb.set_led(locked?RLED:GLED, 300, 300, 255);
      }
    } 
    else if(BIT_CHECK(bitreg,GLOW_MODE)) { //glow mode
      hb.set_led(GLED, 100, 100, 64);
      hb.set_light(CURRENT_LEVEL, 0, NOW);
    } 
    else if(BIT_CHECK(bitreg,GLOW_MODE_JUST_CHANGED)) {
      hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    }

    // holding the button
    if(hb.button_pressed() && !locked) {
      double d = hb.difference_from_down();
      if(BIT_CHECK(bitreg,QUICKSTROBE) 
        || (hb.button_pressed_time() > click 
        && d > 0.10 )) {
        BIT_SET(bitreg,QUICKSTROBE);
        if(treg1+blink_freq_map[0] < time) { 
          treg1 = time; 
          hb.set_light(MAX_LEVEL, 0, 20); 
        }
      }
      if(hb.button_pressed_time() >= glow_mode_time 
        &&d <= 0.1 
        && !BIT_CHECK(bitreg,GLOW_MODE_JUST_CHANGED) 
        && !BIT_CHECK(bitreg,QUICKSTROBE) ) {
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
    i = adjustLED(); //Adjust the led and save it
    if(i>0) {
      DBG(Serial.print("Stored Brightness: "); 
      Serial.println(i));
      stored_brightness = i;
      //The above stored brightness will be saved to EEPROM when we switch off the light
    }
    break;

  case MODE_NIGHTLIGHT: 
    {
      if(!hb.low_voltage_state())
        hb.set_led(RLED, 100, 0, 1);
      if(hb.moved(nightlight_sensitivity)) {
        //Serial.println("Nightlight Moved");
        treg1 = time;
        hb.set_light(CURRENT_LEVEL, nightlight_brightness, 1000);
      } 
      else if(time > treg1 + nightlight_timeout) {
        hb.set_light(CURRENT_LEVEL, 0, 1000);
      }

      i = adjustLED();
      if(i>0) {
        //DBG(Serial.print("Nightlight Brightness: "); Serial.println(i));
        nightlight_brightness = i;
      }
      break; 
    }

  case MODE_BLINK:
    if(hb.button_pressed()) {
      if( hb.button_pressed_time()> click) {
        double d = hb.difference_from_down();
        if(d>=0 && d<=0.99) {
          if(d>=0.25) {
            d = d>0.5 ? 0.5 : d;
            blink_frequency = blink_freq_map[0] + (word)((blink_freq_map[1] - blink_freq_map[0]) * 4 * (0.5-d));
          } 
          else {
            blink_frequency = blink_freq_map[1] + (word)((blink_freq_map[2] - blink_freq_map[1]) * 4 * (0.25-d));
          }
          //DBG(Serial.print("Blink Freq: "); Serial.println(blink_frequency));
          BIT_SET(bitreg,BLOCK_TURNING_OFF);
        }
      }
    }
    if(treg1+blink_frequency < time) { 
      treg1 = time;
      hb.set_light(MAX_LEVEL, 0, 20); 
    }
    break;

  case MODE_SOS:
    if(!hb.light_change_remaining())
    { // we're not currently doing anything, start the next step
      static int current_character = 0;
      static char symbols_remaining = 0;
      static byte pattern = 0;
      const char message[] = "SOS";
      const word morse[] = {	
        0x0300, // S ...
        0x0307, // O ---
        0x0300, // S ...
      };
      const int millisPerBeat = 150;

      if(current_character>=sizeof(message))
      { // we've hit the end of message, turn off.
        //mode = MODE_OFF;
        // reset the current_character, so if we're connected to USB, next printing will still work.
        current_character = 0;
        // return now to skip the following code.
        break;
      }

      if(symbols_remaining <= 0) // we're done printing our last character, get the next!
      {
        // Extract the symbols and length
        pattern = morse[current_character] & 0x00FF;
        symbols_remaining = morse[current_character] >> 8;
        // we count space (between dots/dashes) as a symbol to be printed;
        symbols_remaining *= 2;
        current_character++;
      }

      if (symbols_remaining<=0)
      { // character was unrecognized, treat it as a space
        // 7 beats between words, but 3 have already passed
        // at the end of the last character
        hb.set_light(0,0, millisPerBeat * 4);
      }
      else if (symbols_remaining==1) 
      { // last symbol in character, long pause
        hb.set_light(0, 0, millisPerBeat * 3);
      }
      else if(symbols_remaining%2==1) 
      { // even symbol, print space!
        hb.set_light(0,0, millisPerBeat);
      }
      else if (pattern & 1)
      { // dash, 3 beats
        hb.set_light(MAX_LEVEL, MAX_LEVEL, millisPerBeat * 3);
        pattern >>= 1;
      }
      else 
      { // dot, by elimination
        hb.set_light(MAX_LEVEL, MAX_LEVEL, millisPerBeat);
        pattern >>= 1;
      }
      symbols_remaining--;
    }
    break;
  }  
} 



