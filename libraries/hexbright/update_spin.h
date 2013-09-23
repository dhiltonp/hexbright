#ifdef BUILD_HACK
///////////////////////////////////////////////
///////////overrideable functions//////////////
///////////////////////////////////////////////


#ifndef UPDATE_SPIN
#define UPDATE_SPIN
// default update_spin, only added if an alternative is not requested.

void update_spin() {
  static unsigned long continue_time=0;
  unsigned long now;

#if (DEBUG==DEBUG_LOOP)
  unsigned long start_time=micros();
#endif

  do {
    now = micros();
  } while ((signed long)(continue_time - now) > 0); // not ready for update

  // if we're in debug mode, let us know if our loops are too large
#if (DEBUG!=DEBUG_OFF && DEBUG!=DEBUG_PRINT)
  static int i=0;
#if (DEBUG==DEBUG_LOOP)
  static unsigned long last_time = 0;
  if(!i) {
    Serial.print("Time used: ");
    Serial.print(start_time-last_time);
    Serial.println("/8333");
  }
  last_time = now;
#endif
  if(now-continue_time>5000 && !i) {
    // This may be caused by too much processing for our update_delay, or by too many print statements)
    //  If you're triggering this, your button and light will react more slowly, and some accelerometer
    //  data is being missed.
    Serial.println("WARNING: code is too slow");
  }
  if (!i)
    i=(1000/UPDATE_DELAY); // display loop output every second
  else
    i--;
#endif

  // advance time at the same rate as values are changed in the accelerometer.
  //  advance continue_time here, so the first run through short-circuits, 
  //  meaning we will read hardware immediately after power on.
  continue_time = continue_time+(int)(1000*UPDATE_DELAY);
}

#endif // UPDATE_SPIN



#ifdef STROBE_H 

/// we provide an alternative update_spin function so we can strobe in the background
void update_spin() {
  static unsigned long continue_time=0;
  unsigned long now;

  while (true) {
    do {
      now = micros();
    } while (next_strobe > now && // not ready for strobe
	     continue_time > now); // not ready for update
    if (next_strobe <= now) {
      if (now - next_strobe <26) {
	digitalWriteFast(DPIN_DRV_EN, HIGH);
	delayMicroseconds(strobe_duration);
	digitalWriteFast(DPIN_DRV_EN, LOW);
      }
      next_strobe += strobe_delay;
    }
    if(continue_time <= now) {
      if(strobe_delay>UPDATE_DELAY*1000 && // we strobe less than once every 8333 microseconds
	 next_strobe-continue_time < 4000) // and the next strobe is within 4000 microseconds (may occur before we return)
	continue;
      else
	break;
    }
  } // do nothing... (will short circuit once every 70 minutes (micros maxint))


  // advance time at the same rate as values are changed in the accelerometer.
  //  advance continue_time here, so the first run through short-circuits, 
  //  meaning we will read hardware immediately after power on.
  continue_time = continue_time+(int)(1000*UPDATE_DELAY);
}

#endif // STROBE_H

#endif // BUILD_HACK
