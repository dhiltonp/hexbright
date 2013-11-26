#ifndef PRINT_BINARY_H
#define PRINT_BINARY_H

#include <hexbright.h>
/*
// these 2 includes are needed for print_binary to actually print.
// Arduino is trying to pull in this file when it's not asked for, causing 
//  dependency issues/build errors. 
// Commenting out these includes will mean that any code that calls print_binary
//  without including Serial.h and String.h in the .ino file will just fail to
//  print (instead of printing a warning as designed).
#include <Serial.h>
#include <String.h>
*/

// convenience functions that will print 1 or 2 byte values through the serial port.
extern void print_binary(byte value);
extern void print_binary(int value);

#endif // PRINT_BINARY_H
