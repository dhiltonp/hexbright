#ifndef SOS_MODE_H
#define SOS_MODE_H

#include "hexbright.h"
#include "mode.h"


static int current_character = 0;
static char symbols_remaining = 0;
static byte pattern = 0;
const char message[] = "SOS";
const word morse[] = { 0x0300, // S ...
                       0x0307, // O ---
                       0x0300, // S ...
                      };
const int millisPerBeat = 150;

class sos_mode : public mode {
 public:
  static void start_mode() {
    hexbright::set_light(CURRENT_LEVEL, 0, NOW);
  };

  static void continue_mode() {
    if(!hexbright::light_change_remaining()) { // we're not currently doing anything, start the next step
  
      if(current_character>=sizeof(message))
      { // we've hit the end of message, turn off.
        //mode = MODE_OFF;
        // reset the current_character, so if we're connected to USB, next printing will still work.
        current_character = 0;
        // return now to skip the following code.
        return;
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
        hexbright::set_light(0,0, millisPerBeat * 4);
      }
      else if (symbols_remaining==1) 
      { // last symbol in character, long pause
        hexbright::set_light(0, 0, millisPerBeat * 3);
      }
      else if(symbols_remaining%2==1) 
      { // even symbol, print space!
        hexbright::set_light(0,0, millisPerBeat);
      }
      else if (pattern & 1)
      { // dash, 3 beats
        hexbright::set_light(MAX_LEVEL, MAX_LEVEL, millisPerBeat * 3);
        pattern >>= 1;
      }
      else 
      { // dot, by elimination
        hexbright::set_light(MAX_LEVEL, MAX_LEVEL, millisPerBeat);
        pattern >>= 1;
      }
      symbols_remaining--;
    }
  };

  static void stop_mode() {
    hexbright::set_light(CURRENT_LEVEL, 0, NOW);
  };
};


    

#endif // SOS_MODE_H