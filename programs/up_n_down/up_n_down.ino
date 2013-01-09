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

// EEPROM
#define EEPROM_LAST_ON_MODE 0

 // Defaults
static const int glow_mode_time = 5000;
static const int short_click = 350; // maximum time for a "short" click

static const int nightlight_timeout = 3000; // timeout before nightlight powers down after any movement
static const int nightlight_red_brightness = 255; // brightness of red led for nightlight
static const int nightlight_sensitivity = 20; // measured in 100's of a G.

// State
static bool glow_mode = false;
static bool glow_mode_set = false;

static int mode = MODE_OFF;
static int new_mode = MODE_OFF;  
static int click_count = 0;
static int block_turning_off = false;
static int submode_lockout = 0;

static unsigned long nightlight_move_time;

static boolean blink_random = true;
static int dazzle_odds = 4; // odds of being on are 1:N

static int blink_frequency = 4;
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
  double d;
  int i;

  hb.update();

  // Charging
  if(!hb.printing_number() && !glow_mode)
    hb.print_charge(GLED);
  
  // Check for mode and do in-mode activities
  switch(mode) {
  case MODE_OFF:
    // glow mode
    if(glow_mode)
      hb.set_led(GLED, 100);

    // click counting
    if(hb.button_just_released()) {
      glow_mode_set = false;
      if(hb.button_pressed_time()<=9) {
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

    // toggle glow mode
    if(hb.button_pressed() && hb.button_pressed_time() >= glow_mode_time && !glow_mode_set) {
      glow_mode = !glow_mode;
      glow_mode_set = true;
      if(glow_mode)
	Serial.println("Glow mode");
      else
	Serial.println("Glow mode off");	
    }
    break;
  case MODE_LEVEL:
    if(hb.button_pressed() && hb.button_pressed_time()>short_click) {
      d = hb.difference_from_down();
      if(d>=0.01 && d<=1.0) {
	hb.set_light(CURRENT_LEVEL, (int)(d * 1000.0), NOW);
	block_turning_off = true;
      }
    }
    break;
  case MODE_NIGHTLIGHT:
    if(hb.moved(nightlight_sensitivity)) {
      nightlight_move_time = time;
      hb.set_light(CURRENT_LEVEL, 1000, 1000);
    } else if(time > nightlight_move_time + nightlight_timeout) {
      hb.set_light(CURRENT_LEVEL, 0, 1000);
      if(!hb.printing_number())
	hb.set_led(RLED, 100, 0, nightlight_red_brightness);
    }
    break;
  case MODE_BLINK:
    if(hb.button_pressed() && hb.button_pressed_time()>short_click) {
      d = hb.difference_from_down();
      if(d>=0 && d<=0.99) {
	if(blink_random) {
	  int new_odds = (int)(d*10.0)+2;
	  if(new_odds != dazzle_odds) {
	    dazzle_odds = new_odds;
	    Serial.print("Dazzle Odds: "); Serial.println(dazzle_odds);
	    block_turning_off = true;
	  }
	} else {
	  int new_freq = (int)(d*100.0)-4;
	  if(new_freq<=0)
	    new_freq=1;
	  if(new_freq != blink_frequency) {
	    blink_frequency = new_freq;
	    Serial.print("Blink Freq: "); Serial.println(blink_frequency);
	    block_turning_off = true;
	    submode_lockout = time + short_click;
	  }
	}
      }
    }
    if(hb.tapped() and time > submode_lockout) {
      blink_random = !blink_random;
    }
    if(blink_random) {
      if(random(dazzle_odds)<1)
	hb.set_light(CURRENT_LEVEL, 1000, NOW);
      else
	hb.set_light(CURRENT_LEVEL, 0, NOW);      
    } else {
      if(blink_count>=blink_frequency) {
	i = hb.get_light_level()==0 ? 1000 : 0;
	hb.set_light(CURRENT_LEVEL, i, NOW);
	blink_count=0;
      } else
	blink_count++;
      break;
    }
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
      i = 1000;
      if(d <= 0.40) {
	if(d <= 0.10)
	  i = 1;
	else
	  i = 500;
      }
      hb.set_light(CURRENT_LEVEL, i, NOW); 
      break;
    case MODE_NIGHTLIGHT:
      Serial.println("Mode = nightlight");
      //hb.set_light(CURRENT_LEVEL, 1, NOW);
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
  
