#include <hexbright.h>
#include <Wire.h>
#include <EEPROM.h>

// Modes
#define MAX_MODE 3

#define MODE_OFF 0
#define MODE_LEVEL 1
#define MODE_BLINK 2
#define MODE_NIGHTLIGHT 3

#define MAX_QUICKACCESS_MODE 1

#define QUICKACCESS_BLINK 0
#define QUICKACCESS_LIGHT 1

#define MODE_LOCKED -1

// EEPROM 
#define EEPROM_QUICKACCESS_MODE 0

 // Defaults
static const int glow_mode_time = 3000;
static const int short_click = 350; // maximum time for a "short" click

static const int nightlight_timeout = 3000; // timeout before nightlight powers down after any movement
static const unsigned char nightlight_red_brightness = 255; // brightness of red led for nightlight
static const unsigned char nightlight_sensitivity = 15; // measured in 100's of a G.

// State
static char quickaccess_mode;

static boolean glow_mode = false;
static boolean glow_mode_set = false;

static char mode = MODE_OFF;
static char new_mode = MODE_OFF;  
static char click_count = 0;
static boolean block_turning_off = false;
static unsigned long submode_lockout=0;

static unsigned long nightlight_move_time;

static boolean dazzle = false;
static char dazzle_odds = 4; // odds of being on are 1:N
static char blink_frequency = 70; // in ms
static unsigned long blink_time = millis();

hexbright hb;

void setup() {
  // We just powered on!  That means either we got plugged
  // into USB, or the user is pressing the power button.
  hb = hexbright();
  hb.init_hardware();

  // read current quickaccess mode from EEPROM
  quickaccess_mode = EEPROM.read(EEPROM_QUICKACCESS_MODE);
  if(quickaccess_mode<0 || quickaccess_mode>MAX_QUICKACCESS_MODE)
    quickaccess_mode=QUICKACCESS_BLINK;

  Serial.print("Quickaccess mode defaulted to: "); Serial.println(quickaccess_mode,DEC);
  Serial.println("Powered up!");
} 

void loop() {
  const unsigned long time = millis();
  int i;

  hb.update();

  // Charging
#ifdef PRINTING_NUMBER:
  if(!hb.printing_number()) {
    if(!glow_mode) {
      hb.print_charge(GLED);
    }

    if(hb.low_voltage_state())
      if(hb.get_led_state(RLED)==LED_OFF) 
	hb.set_led(RLED,500,2000);
  }
#else
  if(!glow_mode) {
    hb.print_charge(GLED);
  }

  if(hb.low_voltage_state())
    if(hb.get_led_state(RLED)==LED_OFF) 
      hb.set_led(RLED,500,2000);
#endif

  
 
 // Check for mode and do in-mode activities
  switch(mode) {
  case MODE_OFF:
    // glow mode
    if(glow_mode)
      hb.set_led(GLED, 100);
    
    // click counting
    if(hb.button_just_released()) {
      glow_mode_set = false;
      if(hb.button_pressed_time()<=7) {
	// ignore, could be a bounce
	Serial.println("Bounce!");
      } else if(hb.button_pressed_time() <= short_click) {
	click_count++;
      } 
    } 
    
    // request mode change
    if(click_count && hb.button_released_time() >= short_click) {
      // we're done clicking
      if(click_count > MAX_MODE) {
	click_count = 0;
      }
      new_mode=click_count;
      click_count=0;
    }
    
    // holding the button
    if(hb.button_pressed()) {
      if(!glow_mode_set) {
	double d = hb.difference_from_down();
	if(hb.button_pressed_time() >= glow_mode_time && d <= 0.1) {
	  glow_mode = !glow_mode;
	  glow_mode_set = true;
	  if(glow_mode)
	    Serial.println("Glow mode");
	  else
	    Serial.println("Glow mode off");	
	} else if(hb.button_pressed_time() > short_click && d > 0.10) {
	  // quickaccess mode
	  if(hb.tapped() && time > submode_lockout) {
	    quickaccess_mode = !quickaccess_mode;
	    EEPROM.write(EEPROM_QUICKACCESS_MODE, quickaccess_mode);
	    submode_lockout = time + short_click;
	  }

	  switch(quickaccess_mode) {
	  case QUICKACCESS_BLINK:
	    if(blink_time+blink_frequency < time) { 
	      blink_time = time; 
	      hb.set_light(MAX_LEVEL, 0, 20); 
	    }
	    break;
	  case QUICKACCESS_LIGHT:
	    hb.set_light(MAX_LEVEL, 0, 50); 
	    break;
	  }
	}
      }
    }
    break;
  case MODE_LEVEL:
    if(hb.button_pressed() && hb.button_pressed_time()>short_click) {
      double d = hb.difference_from_down();
      if(d>=0.01 && d<=1.0) {
	i = (int)(d * 2000.0);
	hb.set_light(CURRENT_LEVEL, i>1000?1000:i, 20);
	block_turning_off = true;
      }
    }
    break;
  case MODE_NIGHTLIGHT:
    hb.set_led(RLED, 100, 0, nightlight_red_brightness);
    if(hb.moved(nightlight_sensitivity)) {
      nightlight_move_time = time;
      hb.set_light(CURRENT_LEVEL, 1000, 1000);
    } else if(time > nightlight_move_time + nightlight_timeout) {
      hb.set_light(CURRENT_LEVEL, 0, 1000);
#ifdef PRINTING_NUMBER:
      if(!hb.printing_number())
#endif
    }
    break;
  case MODE_BLINK:
    if(hb.button_pressed()) {
	if( hb.button_pressed_time()>short_click) {
	  double d = hb.difference_from_down();
	  if(d>=0 && d<=0.99) {
	    if(dazzle) {
	      int new_odds = (int)(d*10.0)+2;
	      if(new_odds != dazzle_odds) {
		dazzle_odds = new_odds;
		Serial.print("Dazzle Odds: "); Serial.println(dazzle_odds);
		block_turning_off = true;
		submode_lockout = time + short_click;
	      }
	    } else {
	      blink_frequency = (d*100.0)+20;
	      Serial.print("Blink Freq: "); Serial.println(blink_frequency);
	      block_turning_off = true;
	      submode_lockout = time + short_click;
	    }
	  }
	}
      } else {
	if(hb.tapped() and time > submode_lockout) {
	  Serial.println("Tapped");
	  dazzle = !dazzle;
	  submode_lockout = time + short_click;
	}
      }
      if(dazzle) {
	if(random(dazzle_odds)<1)
	  hb.set_light(MAX_LEVEL, 0, 20);
      } else {
	  if(blink_time+blink_frequency < time) { 
	    blink_time = time;
	    hb.set_light(MAX_LEVEL, 0, 20); 
	  }
      }
      break;
  }  

  // we turn off on button release unless a submode change was made and we're blocked
  if(mode!=MODE_OFF && hb.button_just_released()) {
    if(block_turning_off)
      block_turning_off=false;
    else
      new_mode=MODE_OFF;
  }

  // Do the actual mode change
  if(new_mode!=mode) {
    double d;
    // we're changing mode
    switch(new_mode) {
    case MODE_OFF:
      Serial.println("Mode = off");
      hb.set_light(CURRENT_LEVEL, 0, NOW);
      if(!glow_mode)
	hb.shutdown();
      break;
    case MODE_LEVEL:
      Serial.println("Mode = level");
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
      Serial.println("Mode = nightlight");
      //hb.set_light(CURRENT_LEVEL, 1, NOW);
#ifdef PRINTING_NUMBER:
      if(!hb.printing_number())
#endif
	hb.set_led(RLED, 1000, 0, nightlight_red_brightness);
      break;
    case MODE_BLINK:
      Serial.println("Mode = blink");
      hb.set_light(MAX_LEVEL, 0, 20);
      break;
    }
    mode=new_mode;
  }
}
  
