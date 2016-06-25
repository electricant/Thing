/**
 * Small single-liners useful in many situations
 *
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */

// maximum and minimum functions
#ifndef max
#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
#define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

// round a floating point value to the nearest integer
#define round_uint16(x) ((uint16_t) (x)+0.5)
#define round_int(x) ((x)>=0?(int)(x)+0.5):(int)((x)-0.5))
