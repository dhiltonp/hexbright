#include <hexbright.h>
#include <Wire.h>
#include <EEPROM.h>

// Modes
#define MAX_MODE 4

#define MODE_OFF 0
#define MODE_LEVEL 1
#define MODE_BLINK 2
#define MODE_DAZZLE 3
#define MODE_NIGHTLIGHT 4

#define MODE_LOCKED -1

// EEPROM
#define EEPROM_LAST_ON_MODE 0

 // Defaults
static const int short_click = 300; // maximum time for a "short" click

static const int nightlight_timeout = 3000; // timeout before nightlight powers down after any movement
static const int nightlight_red_brightness = 150; // brightness of red led for nightlight
static const int nightlight_sensitivity = 20; // measured in 100's of a G.

// State
static int mode = MODE_OFF;
static int new_mode = MODE_OFF;  
static int click_count = 0;
static int blockTurningOff = false;

static unsigned long nightlight_move_time;

static int dazzle_odds = 4; // odds of being on are 1:N

static int blink_frequency = 1;
static int blink_count = 0;

hexbright hb;

void setup() {
  // We just powered on!  That means either we got plugged
  // into USB, or the user is pressing the power button.
  hb = hexbright();
  hb.init_hardware();

  /*
  lastOnMode = EEPROM.read(EEPROM_LAST_ON_MODE);
  if (lastOnMode < MODE_LOW || lastOnMode > MODE_HIGH) {
    lastOnMode = MODE_MED;
  }
  */

  Serial.println("Powered up!");
} 

void loop() {
  const unsigned long time = millis();

  hb.update();

  // Charging
  if(!hb.printing_number()) {
    switch (hb.get_charge_state()) {
    case CHARGING:
      if (hb.get_led_state(GLED) == LED_OFF)
	hb.set_led(GLED, 200, 200);
      break;
    case CHARGED:
      hb.set_led(GLED, 10);
      break;
    case BATTERY:
      // led auto offs, don't need to turn it off...
      break;
    }
  }

  // Check for mode and do in-mode activities
  switch(mode) {
  case MODE_OFF:
    if(hb.button_released()) {
      if(hb.button_held_time()<1) {
	// ignore, could be a bounce
	//Serial.println("Bounce!");
      } else if(hb.button_held_time() <= short_click) {
	click_count++;
	//Serial.print("Click Count:"); Serial.println(click_count);
      } 
    } 
    
    if(click_count && hb.button_released_time() >= short_click) {
      // we're done clicking
      if(click_count > MAX_MODE) {
	click_count = 0;
      }
      new_mode=click_count;
      click_count=0;
      //Serial.print("Mode Change: "); Serial.println(new_mode);
    }
    break;
  case MODE_LEVEL:
    if(hb.button_state() && hb.button_held_time()>short_click) {
      double d = hb.difference_from_down();
      if(d>=0.01 && d<=1.0) {
	hb.set_light(CURRENT_LEVEL, (int)(d * 1000.0), NOW);
	blockTurningOff = true;
      }
    }
    break;
  case MODE_NIGHTLIGHT:
    if(hb.moved(nightlight_sensitivity)) {
      nightlight_move_time = time;
      hb.set_light(CURRENT_LEVEL, 1000, 1000);
      //Serial.println("Moved");
    } else if(time > nightlight_move_time + nightlight_timeout) {
      //Serial.println("Move timeout");
      hb.set_light(CURRENT_LEVEL, 1, 1000);
      if(!hb.printing_number())
	hb.set_led(RLED, 100, 0, nightlight_red_brightness);
    }
    break;
  case MODE_DAZZLE:
    if(hb.button_state() && hb.button_held_time()>short_click) {
      double d = hb.difference_from_down();
      if(d>=0 && d<=0.99) {
	int new_odds = (int)(d*10.0)+2;
	if(new_odds != dazzle_odds) {
	  dazzle_odds = new_odds;
	  //Serial.print("Dazzle Odds: "); Serial.println(dazzle_odds);
	  blockTurningOff = true;
	}
      }
    }
    if(random(dazzle_odds)<1)
      hb.set_light(CURRENT_LEVEL, 1000, NOW);
    else
      hb.set_light(CURRENT_LEVEL, 0, NOW);      
    break;
  case MODE_BLINK:
    if(hb.button_state() && hb.button_held_time()>short_click) {
      double d = hb.difference_from_down();
      if(d>=0 && d<=0.99) {
	int new_freq = (int)(d*100.0)-4;
	if(new_freq<=0)
	  new_freq=1;
	if(new_freq != blink_frequency) {
	  blink_frequency = new_freq;
	  //Serial.print("Blink Freq: "); Serial.println(blink_frequency);
	  blockTurningOff = true;
	}
      }
    }
    if(blink_count>=blink_frequency) {
      int level = hb.get_light_level()==0 ? 1000 : 0;
      hb.set_light(CURRENT_LEVEL, level, NOW);
      blink_count=0;
    } else
      blink_count++;
    break;

  }
  
  // we turn off on button release unless a submode change was made and we're blocked
  if(mode!=MODE_OFF && hb.button_released()) {
    if(blockTurningOff)
      blockTurningOff=false;
    else
      new_mode=MODE_OFF;
  }

  // Do the actual mode change
  if(new_mode!=mode) {
    // we're changing mode
    switch(new_mode) {
      case MODE_OFF:
        Serial.println("Mode = off");
        hb.set_light(CURRENT_LEVEL, 0, NOW);
	hb.shutdown();
        break;
      case MODE_LEVEL:
        Serial.println("Mode = level");
	hb.set_light(CURRENT_LEVEL, hb.difference_from_down()<0.25 ? 1 : 1000, NOW); 
	break;
      case MODE_NIGHTLIGHT:
        Serial.println("Mode = nightlight");
	hb.set_light(CURRENT_LEVEL, 1, NOW);
	if(!hb.printing_number())
	  hb.set_led(RLED, 100, 0, nightlight_red_brightness);
	break;
      case MODE_BLINK:
        Serial.println("Mode = blink");
	hb.set_light(CURRENT_LEVEL, 1000, NOW);
	break;
    }
    mode=new_mode;
  }
}

