/*
  Hexbright demo firmware: Morse
  Andrew Magill  9/2012
  
  On each button press, flashes out a message in morse code
  and then turns off.  Message and speed are configurable 
  just below.
*/

char message[] = "SOS HEXBRIGHT SOS";
int millisPerBeat = 75;


// Pin assignments
#define DPIN_RLED_SW            2
#define DPIN_GLED               5
#define DPIN_PWR                8
#define DPIN_DRV_MODE           9
#define DPIN_DRV_EN             10

// High byte = length
// Low byte  = morse code, LSB first, 0=dot 1=dash
word morse[] = {
  0x0202, // A .-
  0x0401, // B -...
  0x0405, // C -.-.
  0x0301, // D -..
  0x0100, // E .
  0x0404, // F ..-.
  0x0303, // G --.
  0x0400, // H ....
  0x0200, // I ..
  0x040E, // J .---
  0x0305, // K -.-
  0x0402, // L .-..
  0x0203, // M --
  0x0201, // N -.
  0x0307, // O ---
  0x0406, // P .--.
  0x040B, // Q --.-
  0x0302, // R .-.
  0x0300, // S ...
  0x0101, // T -
  0x0304, // U ..-
  0x0408, // V ...-
  0x0306, // W .--
  0x0409, // X -..-
  0x040D, // Y -.--
  0x0403, // Z --..
  0x051F, // 0 -----
  0x051E, // 1 .----
  0x051C, // 2 ..---
  0x0518, // 3 ...--
  0x0510, // 4 ....-
  0x0500, // 5 .....
  0x0501, // 6 -....
  0x0503, // 7 --...
  0x0507, // 8 ---..
  0x050F, // 9 ----.
};

void setup()
{
  pinMode(DPIN_PWR,      INPUT);
  digitalWrite(DPIN_PWR, LOW);

  // Initialize GPIO
  pinMode(DPIN_RLED_SW,  INPUT);
  pinMode(DPIN_GLED,     OUTPUT);
  pinMode(DPIN_DRV_MODE, OUTPUT);
  pinMode(DPIN_DRV_EN,   OUTPUT);
  digitalWrite(DPIN_DRV_MODE, LOW);
  digitalWrite(DPIN_DRV_EN,   LOW);
}

void loop()
{
  // Expect a button press to start
  if (!digitalRead(DPIN_RLED_SW)) return;

  // Ensure the regulator stays enabled
  pinMode(DPIN_PWR,      OUTPUT);
  digitalWrite(DPIN_PWR, HIGH);
  
  for (int i = 0; i < sizeof(message); i++)
  {
    char ch = message[i];
    if (ch == ' ')
    {
      // 7 beats between words, but 3 have already passed
      // at the end of the last character
      delay(millisPerBeat * 4);
    }
    else
    {
      // Remap ASCII to the morse table
      if      (ch >= 'A' && ch <= 'Z') ch -= 'A';
      else if (ch >= 'a' && ch <= 'z') ch -= 'a';
      else if (ch >= '0' && ch <= '9') ch -= '0' - 26;
      else continue;
      
      // Extract the symbols and length
      byte curChar  = morse[ch] & 0x00FF;
      byte nSymbols = morse[ch] >> 8;
      
      // Play each symbol
      for (int j = 0; j < nSymbols; j++)
      {
        digitalWrite(DPIN_DRV_EN, HIGH);
        if (curChar & 1)  // Dash - 3 beats
          delay(millisPerBeat * 3);
        else              // Dot - 1 beat
          delay(millisPerBeat);
        digitalWrite(DPIN_DRV_EN, LOW);
        // One beat between symbols
        delay(millisPerBeat);
        curChar >>= 1;
      }
      // 3 beats between characters, but one already
      // passed after the last symbol.
      delay(millisPerBeat * 2);
    } 
  }
  
  // Flash the green LED to indicate we're done
  for (int k = 0; k < 3; k++)
  {
    digitalWrite(DPIN_GLED, HIGH);
    delay(50);
    digitalWrite(DPIN_GLED, LOW);
    delay(50);    
  }
  // Disable the regulator to power off
  // This won't work on USB power.
  digitalWrite(DPIN_PWR, LOW);
}

