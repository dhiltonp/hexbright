/* 

  Factory firmware for HexBright FLEX 
  v2.4  Dec 6, 2012
  
*/

#include <math.h>
#include <Wire.h>

/*
// Settings
#define OVERTEMP                340
// Pin assignments
#define DPIN_RLED_SW            2
#define DPIN_GLED               5
#define DPIN_PWR                8
#define DPIN_DRV_MODE           9
#define DPIN_DRV_EN             10
#define APIN_TEMP               0
#define APIN_CHARGE             3
*/
// pin settings taken care of inside the library


// Conversion notes:
// build size: 
// 8920 bytes, no debugging, but accelerometer and number printing enabled 
//  (#define PRINT_NUMBER and ACCELEROMETER at the top of hexbright.h)
// 7000 bytes with those disabled
// original was 5866, so the use of the library costs us approximately 1133 bytes,
//  but we get some additional features that aren't used in this program.

// In general, a code section is block commented out /* */, followed by a short 
//  description and the alternative.

#include <hexbright.h>

// Modes
#define MODE_OFF                0
#define MODE_LOW                1
#define MODE_MED                2
#define MODE_HIGH               3
#define MODE_BLINKING           4
#define MODE_BLINKING_PREVIEW   5

// State
byte mode = 0;
unsigned long btnTime = 0;
boolean btnDown = false;

hexbright hb;

void setup()
{
/*
  // We just powered on!  That means either we got plugged 
  // into USB, or the user is pressing the power button.
  pinMode(DPIN_PWR,      INPUT);
  digitalWrite(DPIN_PWR, LOW);

  // Initialize GPIO
  pinMode(DPIN_RLED_SW,  INPUT);
  pinMode(DPIN_GLED,     OUTPUT);
  pinMode(DPIN_DRV_MODE, OUTPUT);
  pinMode(DPIN_DRV_EN,   OUTPUT);
  digitalWrite(DPIN_DRV_MODE, LOW);
  digitalWrite(DPIN_DRV_EN,   LOW);
  
  // Initialize serial busses
  Serial.begin(9600);
  Wire.begin();
  
  btnTime = millis();
  btnDown = digitalRead(DPIN_RLED_SW);
  mode = MODE_OFF;

  Serial.println("Powered up!");
*/
  /// Set DEBUG to a non-zero value in hexbright.h to enable printing
  hb.init_hardware();
}

void loop()
{
  // read values, adjust lights, etc.
  hb.update();
  
/*  
  static unsigned long lastTempTime;
  unsigned long time = millis();

  // Check the state of the charge controller
  int chargeState = analogRead(APIN_CHARGE);
  if (chargeState < 128)  // Low - charging
  {
    digitalWrite(DPIN_GLED, (time&0x0100)?LOW:HIGH);
  }
  else if (chargeState > 768) // High - charged
  {
    digitalWrite(DPIN_GLED, HIGH);
  }
  else // Hi-Z - shutdown
  {
    digitalWrite(DPIN_GLED, LOW);    
  }*/
  switch(hb.get_charge_state())
  {
  case CHARGING:
    if(hb.get_led_state(GLED)==LED_OFF)
      hb.set_led(GLED, 200, 200);
    break;
  case CHARGED:
    hb.set_led(GLED, 10);
    break;
  case BATTERY:
    // led auto offs, don't need to turn it off...
    break;
  };
  
  /*
  // Check the temperature sensor
  if (time-lastTempTime > 1000)
  {
    lastTempTime = time;
    int temperature = analogRead(APIN_TEMP);
    Serial.print("Temp: ");
    Serial.println(temperature);
    if (temperature > OVERTEMP && mode != MODE_OFF)
    {
      Serial.println("Overheating!");

      for (int i = 0; i < 6; i++)
      {
        digitalWrite(DPIN_DRV_MODE, LOW);
        delay(100);
        digitalWrite(DPIN_DRV_MODE, HIGH);
        delay(100);
      }
      digitalWrite(DPIN_DRV_MODE, LOW);

      mode = MODE_LOW;
    }
  }
*/
  // overheat code is already handled internal to the library.
  // you can read the reading with hb.get_thermal_sensor(), 
  // hb.get_celsius(), or hb.get_fahrenheit().

/* 
  // Do whatever this mode does
  switch (mode)
  {
  case MODE_BLINKING:
  case MODE_BLINKING_PREVIEW:
    digitalWrite(DPIN_DRV_EN, (time%300)<75);
    break;
  }
*/
  // once every 40 times we come through here, have the light pulse for 50 ms
  if(mode == MODE_BLINKING || mode == MODE_BLINKING_PREVIEW) 
  {
    static int i = 0;
    i = (i+1)%40;
    if(i==0) {
      hb.set_light(MAX_LEVEL, 0, 50);
    }
  }
  
/*  
  // Periodically pull down the button's pin, since
  // in certain hardware revisions it can float.
  pinMode(DPIN_RLED_SW, OUTPUT);
  pinMode(DPIN_RLED_SW, INPUT);
*/
  // the library automatically handles the RLED/switch

  // Check for mode changes
  byte newMode = mode;
/*  
  byte newBtnDown = digitalRead(DPIN_RLED_SW);
*/
  // mode setting is still handled in the main program.  For now, we'll 
  //  just translate the button presses.
  int button_time_held = hb.button_held();
  boolean button_released = hb.button_released();
  
  switch (mode)
  {
  case MODE_OFF:
    if (button_time_held>500)
      newMode = MODE_BLINKING_PREVIEW;
    else if (button_released)
      newMode = MODE_LOW;
    break;
  case MODE_LOW:
    if (button_released)
      newMode = MODE_MED;
    break;
  case MODE_MED:
    if (button_released)
      newMode = MODE_HIGH;
    break;
  case MODE_HIGH:
    if (button_released)
      newMode = MODE_OFF;
    break;
  case MODE_BLINKING_PREVIEW:
    // This mode exists just to ignore this button release.
    if (button_released)
      newMode = MODE_BLINKING;
    break;
  case MODE_BLINKING:
    if (button_released)
      newMode = MODE_OFF;
    break;
  }

  // Do the mode transitions
  if (newMode != mode)
  {
    switch (newMode)
    {
    case MODE_OFF:
      Serial.println("Mode = off");
/*      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, LOW);
      digitalWrite(DPIN_DRV_MODE, LOW);
      digitalWrite(DPIN_DRV_EN, LOW);
*/    hb.shutdown();
      break;
    case MODE_LOW:
      Serial.println("Mode = low");
/*      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, LOW);
      analogWrite(DPIN_DRV_EN, 64);
*/    hb.set_light(CURRENT_LEVEL, 200, NOW);
      break;
    case MODE_MED:
      Serial.println("Mode = medium");
/*      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, LOW);
      analogWrite(DPIN_DRV_EN, 255);
*/    hb.set_light(CURRENT_LEVEL, MAX_LOW_LEVEL, NOW);
      break;
    case MODE_HIGH:
      Serial.println("Mode = high");
/*      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, HIGH);
      analogWrite(DPIN_DRV_EN, 255);
*/    hb.set_light(CURRENT_LEVEL, MAX_LEVEL, NOW);
      break;
    case MODE_BLINKING:
    case MODE_BLINKING_PREVIEW:
      Serial.println("Mode = blinking");
/*      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, HIGH);
*/    // already taken care of
      break;
    }

    mode = newMode;
  }

/*
  // Remember button state so we can detect transitions
  if (newBtnDown != btnDown)
  {
    btnTime = time;
    btnDown = newBtnDown;
    delay(50);
  }
*/
  // button state is maintained inside the library.
}

