/**
 * Small single-liners useful in many situations
 *
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */

// maximum and minimum functions
#ifndef max
#define max(a, b) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
#define min(a, b) ( ((a) < (b)) ? (a) : (b) )
#endif

// round a floating point value to the nearest integer
#define round_uint16(x) ((uint16_t) (x)+0.5)
#define round_int(x) ((x)>=0?(int)(x)+0.5):(int)((x)-0.5))

// For buffer handling we need a modulo operation that can work with negative
// numbers. Just add the divisor and we're done.
// For more information see:
// http://stackoverflow.com/questions/4003232/how-to-code-a-modulo-operator-in-c-c-obj-c-that-handles-negative-numbers
#define mod(x, N) ( ((x) + (N)) % (N) )
