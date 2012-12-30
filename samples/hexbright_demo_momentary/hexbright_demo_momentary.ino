/*
  Hexbright demo firmware: Momentary
  Andrew Magill  9/2012
  
  Light turns on when the button is down.  That's it.
*/

// Pin assignments
#define DPIN_RLED_SW            2
#define DPIN_GLED               5
#define DPIN_PWR                8
#define DPIN_DRV_MODE           9
#define DPIN_DRV_EN             10


void setup()
{
  // If we leave the regulator's enable pin as high-impedance,
  // the regulator will power down the board about a half 
  // second after release of the button.
  pinMode(DPIN_PWR,      INPUT);
  digitalWrite(DPIN_PWR, LOW);

  // Initialize GPIO
  pinMode(DPIN_RLED_SW,  INPUT);
  pinMode(DPIN_GLED,     OUTPUT);
  pinMode(DPIN_DRV_MODE, OUTPUT);
  pinMode(DPIN_DRV_EN,   OUTPUT);
  digitalWrite(DPIN_DRV_MODE, HIGH);
  digitalWrite(DPIN_DRV_EN,   LOW);
  digitalWrite(DPIN_GLED, HIGH);
}

void loop()
{
  if (digitalRead(DPIN_RLED_SW))
    digitalWrite(DPIN_DRV_EN, HIGH);
  else
    digitalWrite(DPIN_DRV_EN, LOW);
}

