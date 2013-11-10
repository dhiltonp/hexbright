// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>
#include <EEPROM.h>

hexbright hb;

/*
  When plugged in:
    Print the contents of EEPROM, then go to the OFF state (as if we were unplugged).
    magnitude (100 = 1g): x,y,z (acceleration vector in gs (100 = 1g))

  When unplugged:
    button press: put in a wait state (low light), where any significant action will start the recording
     This will increase the light output for the duration of the recording, about 1.4 seconds.
     When the recording is finished, turn off.
*/

#define OFF_MODE 0
#define WAIT_MODE 1
#define RECORD_MODE 2
#define PRINT_MODE 3
int mode = OFF_MODE;

#define EEPROM_SIZE 510 // last two bytes can't fit a whole vector...
int address = EEPROM_SIZE;

void setup() {
  hb.init_hardware();
  if(hb.get_charge_state()!=BATTERY) {
    // we're plugged in, print
    Serial.println("magnitude: x/y/z");
    mode = PRINT_MODE;
    address = 0;
  }
}


void loop() {
  hb.update();
  switch (mode) {
  case OFF_MODE:
    if(hb.button_just_released()) {
      hb.set_light(100, 100, NOW);
      address = 0;
      mode = WAIT_MODE;
    }
    break;
  case WAIT_MODE:
    // we've had a fairly significant change in the vector
    if(abs(hb.magnitude(hb.vector(0)))-100>15) {
      hb.set_light(500, 500, NOW);
      mode = RECORD_MODE;
      write_vector(hb.vector(3));
      write_vector(hb.vector(2));
      write_vector(hb.vector(1));
      write_vector(hb.vector(0));
    }
    break;     
  case RECORD_MODE:
    if(address<EEPROM_SIZE) {
      write_vector(hb.vector(0)); 
    } else {
      hb.set_light(0, 0, NOW);
      mode = OFF_MODE;
    }
    break;
  case PRINT_MODE:
    int vec[3];
    if(address<EEPROM_SIZE) {
      read_vector(vec);
      // process vector, print useful info
      Serial.print(hb.magnitude(vec));
      Serial.print(": ");
      hb.print_vector(vec, "recorded vector");
    } else {
      Serial.println("DONE");
      mode = OFF_MODE; 
    }
  }
}

// this writes a 6 bit value into an 8 bit space.
//  We could optimize this, giving us 1.8 seconds instead of 1.4 seconds of data by 
//  manually managing the byte boundaries.  For now, 1.4 seconds is good enough.
void write_vector(int * vector) {
  if(address<EEPROM_SIZE) {
    char tmp = 0;
    for (int i = 0; i<3; i++) {
      if(vector[i] == 0) 
        tmp = 0;
      else
        tmp = vector[i]*(21.3/100)+1; // +1 compensates for flooring from double-integer conversion.
      EEPROM.write(address++, tmp);
    }
  } 
}

void read_vector(int * vector) {
  if(address<EEPROM_SIZE) {
    char tmp = 0;
    for (int i = 0; i<3; i++) {
      tmp = EEPROM.read(address++);
       // we lose some information with the repeated integer conversions; this fixes it.
       //  the alternative is to modify read_accelerometer to store the raw data,
       //  but then the vector math methods doesn't work right.
      vector[i] = tmp*(100/21.3);
    }
  }  
}
