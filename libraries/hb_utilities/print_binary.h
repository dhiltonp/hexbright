#ifndef PRINT_BINARY_H
#define PRINT_BINARY_H

#include <hexbright.h>
#include <Serial.h>
#include <String.h>

// convenience functions that will print 1 or 2 byte values through the serial port.
extern void print_binary(byte value);
extern void print_binary(int value);

#endif // PRINT_BINARY_H
