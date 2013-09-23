Part of the lower-level arduino library, copied from:

https://github.com/arduino/Arduino/tree/master/libraries/Wire/utility

I have stripped out all slave code and reduced the buffer size to the maximum we currently need. This has resulted in significant savings; 400 more bytes of flash (beyond the 500 gained from switching from Wire.h), and 100 more bytes of ram available (beyond the 100 gained from removing Wire.h).
