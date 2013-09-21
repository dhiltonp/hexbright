#include "click_counter.h"

byte clickState;
byte clickCount;
word max_click_time;

void config_click_count(word click_time) {
  max_click_time = click_time;
  clickState=0;
}

char click_count() {
  switch(clickState) {
  case CLICK_OFF:
    if(hexbright::button_just_pressed()) {
      // click just started
      clickState = CLICK_ACTIVE;
      clickCount=0;
      //Serial.println("Clicking just started");
    }
    break;
  case CLICK_ACTIVE:   
    if(hexbright::button_just_released()) {
      if(hexbright::button_pressed_time() > max_click_time) {
	// button held to long
	//Serial.println("Click held too long");
	clickState = CLICK_OFF;
      } else {
	// click is counted
	clickState = CLICK_WAIT;
	clickCount++;
	//Serial.println("Click counted");
      }
    }
    break;
  case CLICK_WAIT:
    // button is released for long enough, we're done clicking
    if(hexbright::button_released_time() > max_click_time) {
      clickState = CLICK_OFF;
      //Serial.print("Click finished: "); Serial.println((int)clickCount);
      return clickCount;
    } else if(hexbright::button_pressed()) {
      // move back to active state
      clickState = CLICK_ACTIVE;
      //Serial.println("Click active");
    }
  }
  return -127;
} 
