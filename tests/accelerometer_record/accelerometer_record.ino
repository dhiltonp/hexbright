#include <hexbright.h>
#include <Wire.h>
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
  if(hb.get_definite_charge_state()!=BATTERY) {
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
    if(hb.button_released()) {
      hb.set_light(100, 100, NOW);
      address = 0;
      mode = WAIT_MODE;
    }
    break;
  case WAIT_MODE:
    if(abs(hb.magnitude(hb.vector(0)))-100>15) {
      // don't break, so we get this sample as well
      hb.set_light(500, 500, NOW);
      mode = RECORD_MODE;
      write_vector(hb.vector(3));
      write_vector(hb.vector(2));
      write_vector(hb.vector(1));
    } else { // we're not ready to record, break!
      break; 
    }    
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

void write_vector(int * vector) {
  if(address<EEPROM_SIZE) {
    char tmp = 0;
    for (int i = 0; i<3; i++) {
      if(vector[i] == 0) 
        tmp = 0;
      else 
        tmp = vector[i]*(21.3/100)+1;
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
