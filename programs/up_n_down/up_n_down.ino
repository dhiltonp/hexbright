#include <hexbright.h>
#include <Wire.h>
#include <EEPROM.h>

// Modes
#define MAX_MODE 3

#define MODE_OFF 0
#define MODE_LEVEL 1
#define MODE_BLINK 2
#define MODE_NIGHTLIGHT 3

#define MODE_LOCKED -1

 // Defaults
static const int glow_mode_time = 3000;
static const int short_click = 350; // maximum time for a "short" click
static const int nightlight_timeout = 10000; // timeout before nightlight powers down after any movement
static const unsigned char nightlight_sensitivity = 20; // measured in 100's of a G.

// State
static unsigned long treg1=0; 

static unsigned int bitreg=0;
const unsigned BLOCK_TURNING_OFF=0;
const unsigned GLOW_MODE=1;
const unsigned GLOW_MODE_SET=2;
const unsigned DAZZLE=3;

static char mode = MODE_OFF;
static char new_mode = MODE_OFF;  
static char click_count = 0;

static unsigned long submode_lockout=0;

static char dazzle_odds = 4; // odds of being on are 1:N
static char blink_frequency = 70; // in ms

hexbright hb;

void setup() {
  // We just powered on!  That means either we got plugged
  // into USB, or the user is pressing the power button.
  hb = hexbright();
  hb.init_hardware();

  Serial.println("Powered up!");
} 

void loop() {
  const unsigned long time = millis();
  int i;

  hb.update();

  // Charging
#ifdef PRINTING_NUMBER:
  if(!hb.printing_number()) 
#endif
  {
    if(!(bitreg & (1<<GLOW_MODE))) {
      hb.print_charge(GLED);
    }

    if(mode != MODE_OFF && hb.low_voltage_state())
      if(hb.get_led_state(RLED)==LED_OFF) 
	hb.set_led(RLED,50,1000);
  }
 
 // Check for mode and do in-mode activities
  switch(mode) {
  case MODE_OFF:
    // glow mode
    if(bitreg & (1<<GLOW_MODE))
      hb.set_led(GLED, 100);
    
    // click counting
    if(hb.button_just_released()) {
      bitreg &= ~(1<<GLOW_MODE_SET);
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
      if(!(bitreg & (1<<GLOW_MODE_SET))) {
	double d = hb.difference_from_down();
	if(hb.button_pressed_time() >= glow_mode_time && d <= 0.1) {
	  bitreg ^= (1<<GLOW_MODE);
	  bitreg |= (1<<GLOW_MODE_SET);
	  if(bitreg & (1<<GLOW_MODE))
	    Serial.println("Glow mode");
	  else
	    Serial.println("Glow mode off");	
	} else if(hb.button_pressed_time() > short_click && d > 0.10) {
	  // quickstrobe
	  if(treg1+blink_frequency < time) { 
	      treg1 = time; 
	      hb.set_light(MAX_LEVEL, 0, 20); 
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
	bitreg |= (1<<BLOCK_TURNING_OFF);
      }
    }
    break;
  case MODE_NIGHTLIGHT:
    if(!hb.low_voltage_state())
      hb.set_led(RLED, 100, 0);
    if(hb.moved(nightlight_sensitivity)) {
      treg1 = time;
      hb.set_light(CURRENT_LEVEL, 1000, 1000);
    } else if(time > treg1 + nightlight_timeout) {
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
	    if(bitreg & (1<<DAZZLE)) {
	      int new_odds = (int)(d*10.0)+2;
	      if(new_odds != dazzle_odds) {
		dazzle_odds = new_odds;
		Serial.print("Dazzle Odds: "); Serial.println(dazzle_odds);
		bitreg |= (1<<BLOCK_TURNING_OFF);
		submode_lockout = time + short_click;
	      }
	    } else {
	      blink_frequency = (d*100.0)+20;
	      Serial.print("Blink Freq: "); Serial.println(blink_frequency);
	      bitreg |= (1<<BLOCK_TURNING_OFF);
	      submode_lockout = time + short_click;
	    }
	  }
	}
      } else {
	if(hb.tapped() and time > submode_lockout) {
	  Serial.println("Tapped");
	  bitreg ^= (1<<DAZZLE);
	  submode_lockout = time + short_click;
	}
      }
      if(bitreg & (1<<DAZZLE)) {
	if(random(dazzle_odds)<1)
	  hb.set_light(MAX_LEVEL, 0, 20);
      } else {
	  if(treg1+blink_frequency < time) { 
	    treg1 = time;
	    hb.set_light(MAX_LEVEL, 0, 20); 
	  }
      }
      break;
  }  

  // we turn off on button release unless a submode change was made and we're blocked
  if(mode!=MODE_OFF && hb.button_just_released()) {
    if(bitreg & (1<<BLOCK_TURNING_OFF))
      bitreg &= ~(1<<BLOCK_TURNING_OFF);
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
      if(!(bitreg & (1<<GLOW_MODE)))
	hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
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
	hb.set_led(RLED, 1000, 0);
      break;
    case MODE_BLINK:
      Serial.println("Mode = blink");
      hb.set_light(MAX_LEVEL, 0, 20);
      break;
    }
    mode=new_mode;
  }
}
  
