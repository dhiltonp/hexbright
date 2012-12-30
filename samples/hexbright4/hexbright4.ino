/*             Test firmware for HexBright

Notes:
  Requires Arduino 1.0.1!

*/

#include <math.h>
#include <Wire.h>
#include <hexbright.h>
// State
byte mode = 0;
unsigned long btnTime = 0;
boolean btnDown = false;

hexbright hb;

void setup()
{
  hb.init_hardware();
}

void loop()
{
  static unsigned long lastTime, lastTempTime, lastAccTime;
  static float lastKnobAngle, knob;
  static byte blink;
  unsigned long time = millis();
  
  // Blink the indicator LED now and then
  digitalWrite(DPIN_GLED, (time&0x03FF)?LOW:HIGH);

  // Check the serial port
  if (Serial.available())
  {
    char c = Serial.read();
    switch (c)
    {
    case 's':
      {
        int temperature = analogRead(APIN_TEMP);
        Serial.print("Temperature = ");
        Serial.println(temperature);

        char accel[3];
        readAccel(accel);
        Serial.print("Acceleration = ");
        Serial.print(accel[0], DEC);
        Serial.print(", ");
        Serial.print(accel[1], DEC);
        Serial.print(", ");
        Serial.println(accel[2], DEC);
      
        byte pgood = digitalRead(DPIN_PGOOD);
        Serial.print("LED driver power good = ");
        Serial.println(pgood?"Yes":"No");
      }
      break;
    }
  }

  // Check the temperature sensor
  if (time-lastTempTime > 1000)
  {
    lastTempTime = time;
    int temperature = analogRead(APIN_TEMP);
    Serial.print("Temperature = ");
    Serial.println(temperature);
    if (temperature > OVERTEMP)
    {
      Serial.println("Overheat shutdown!");
      mode = MODE_OFF;
      digitalWrite(DPIN_DRV_MODE, LOW);
      digitalWrite(DPIN_DRV_EN, LOW);
      digitalWrite(DPIN_PWR, LOW);
    }
  }

  // Check if the accelerometer wants to interrupt
  byte tapped = 0, shaked = 0;
  if (!digitalRead(DPIN_ACC_INT))
  {
    Wire.beginTransmission(ACC_ADDRESS);
    Wire.write(ACC_REG_TILT);
    Wire.endTransmission(false);       // End, but do not stop!
    Wire.requestFrom(ACC_ADDRESS, 1);  // This one stops.
    byte tilt = Wire.read();
    
    if (time-lastAccTime > 500)
    {
      lastAccTime = time;
  
      tapped = !!(tilt & 0x20);
      shaked = !!(tilt & 0x80);
  
      if (tapped) Serial.println("Tap!");
      if (shaked) Serial.println("Shake!");
    }
  }

  // Do whatever this mode does
  switch (mode)
  {
  case MODE_KNOBBING:
    {
      if (time-lastTime < 100) break;
      lastTime = time;
      
      float angle = readAccelAngleXZ();
      float change = angle - lastKnobAngle;
      lastKnobAngle = angle;
      if (change >  PI) change -= 2.0*PI;
      if (change < -PI) change += 2.0*PI;
      knob += -change * 40.0;
      if (knob < 0)   knob = 0;
      if (knob > 255) knob = 255;

      // Make apparent brightness changes linear by squaring the
      // value and dividing back down into range.  This gives us
      // a gamma correction of 2.0, which is close enough.
      byte bright = (long)(knob * knob) >> 8;
      // Avoid ever appearing off in this mode!
      if (bright < 8) bright = 8;
      analogWrite(DPIN_DRV_EN, bright);
  
      Serial.print("Ang = ");
      Serial.print(angle);
      Serial.print("\tChange = ");
      Serial.print(change);
      Serial.print("\tKnob = ");
      Serial.print(knob);
      Serial.print("\tBright = ");
      Serial.println(bright);
    }
    break;
  case MODE_BLINKING:
  case MODE_BLINKING_PREVIEW:
    if (time-lastTime < 250) break;
    lastTime = time;

    blink = !blink;
    digitalWrite(DPIN_DRV_EN, blink);
    break;
  case MODE_DAZZLING:
  case MODE_DAZZLING_PREVIEW:
    if (time-lastTime < 10) break;
    lastTime = time;
    
    digitalWrite(DPIN_DRV_EN, random(4)<1);
    break;
  }
  
  // Check for mode changes
  byte newMode = mode;
  byte newBtnDown = digitalRead(DPIN_RLED_SW);
  switch (mode)
  {
  case MODE_OFF:
    if (btnDown && !newBtnDown)  // Button released
      newMode = MODE_LOW;
    if (btnDown && newBtnDown && (time-btnTime)>500)  // Held
      newMode = MODE_KNOBBING;
    break;
  case MODE_LOW:
    if (btnDown && !newBtnDown)  // Button released
      newMode = MODE_HIGH;
    if (btnDown && newBtnDown && (time-btnTime)>500)  // Held
      newMode = MODE_KNOBBING;
    break;
  case MODE_HIGH:
    if (btnDown && !newBtnDown)  // Button released
      newMode = MODE_OFF;
    if (btnDown && newBtnDown && (time-btnTime)>500)  // Held
      newMode = MODE_KNOBBING;
    break;
  case MODE_KNOBBING:
    if (btnDown && !newBtnDown)  // Button released
      newMode = MODE_KNOBBED;
    if (btnDown && newBtnDown && tapped)
      newMode = MODE_BLINKING_PREVIEW;
    break;
  case MODE_KNOBBED:
    if (btnDown && !newBtnDown)  // Button released
      newMode = MODE_OFF;
    if (btnDown && newBtnDown && (time-btnTime)>500)  // Held
      newMode = MODE_KNOBBING;
    break;
  case MODE_BLINKING:
    if (btnDown && !newBtnDown)  // Button released
      newMode = MODE_OFF;
    if (btnDown && newBtnDown && (time-btnTime)>500)  // Held
      newMode = MODE_BLINKING_PREVIEW;
    break;
  case MODE_BLINKING_PREVIEW:
    if (btnDown && !newBtnDown)  // Button released
      newMode = MODE_BLINKING;
    if (btnDown && newBtnDown && tapped)
      newMode = MODE_DAZZLING_PREVIEW;
    break;
  case MODE_DAZZLING:
    if (btnDown && !newBtnDown)  // Button released
      newMode = MODE_OFF;
    if (btnDown && newBtnDown && (time-btnTime)>500)  // Held
      newMode = MODE_DAZZLING_PREVIEW;
    break;
  case MODE_DAZZLING_PREVIEW:
    if (btnDown && !newBtnDown)  // Button released
      newMode = MODE_DAZZLING;
    if (btnDown && newBtnDown && tapped)
      newMode = MODE_BLINKING_PREVIEW;
    break;
  }

  // Do the mode transitions
  if (newMode != mode)
  {
    switch (newMode)
    {
    case MODE_OFF:
      Serial.println("Mode = off");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, LOW);
      digitalWrite(DPIN_DRV_MODE, LOW);
      digitalWrite(DPIN_DRV_EN, LOW);
      break;
    case MODE_LOW:
      Serial.println("Mode = low");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, LOW);
      analogWrite(DPIN_DRV_EN, 255);
      break;
    case MODE_HIGH:
      Serial.println("Mode = high");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, HIGH);
      analogWrite(DPIN_DRV_EN, 255);
      break;
    case MODE_KNOBBING:
      Serial.println("Mode = knobbing");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      lastKnobAngle = readAccelAngleXZ();
      knob = (mode==MODE_OFF) ? 0 : 255;
      break;
    case MODE_KNOBBED:
      Serial.println("Mode = knobbed");
      break;
    case MODE_BLINKING:
    case MODE_BLINKING_PREVIEW:
      Serial.println("Mode = blinking");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, LOW);
      break;
    case MODE_DAZZLING:
    case MODE_DAZZLING_PREVIEW:
      Serial.println("Mode = dazzling");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, HIGH);
      break;
    }

    mode = newMode;
  }

  // Remember button state so we can detect transitions
  if (newBtnDown != btnDown)
  {
    btnTime = time;
    btnDown = newBtnDown;
    delay(50);
  }
}

void readAccel(char *acc)
{
  while (1)
  {
    Wire.beginTransmission(ACC_ADDRESS);
    Wire.write(ACC_REG_XOUT);
    Wire.endTransmission(false);       // End, but do not stop!
    Wire.requestFrom(ACC_ADDRESS, 3);  // This one stops.

    for (int i = 0; i < 3; i++)
    {
      if (!Wire.available())
        continue;
      acc[i] = Wire.read();
      if (acc[i] & 0x40)  // Indicates failed read; redo!
        continue;
      if (acc[i] & 0x20)  // Sign-extend
        acc[i] |= 0xC0;
    }
    break;
  }
}

float readAccelAngleXZ()
{
  char acc[3];
  readAccel(acc);
  return atan2(acc[0], acc[2]);
}

