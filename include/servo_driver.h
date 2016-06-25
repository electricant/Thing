/**
 * 
 * 
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

	#include <stdint.h>

	/**
	 * Define where the various servos are connected and their function
	 */
	#define THUMB_FINGER  1
	#define INDEX_FINGER  2
	#define MIDDLE_FINGER 3
	#define RING_FINGER   4
	#define PINKY_FINGER  5

	#define thumbSetCompare(_comp) TC_SetCompareA( &TCD0, _comp )
	#define indexSetCompare(_comp) TC_SetCompareB( &TCD0, _comp )
	#define middleSetCompare(_comp) TC_SetCompareC( &TCD0, _comp )
	#define ringSetCompare(_comp) TC_SetCompareA( &TCE0, _comp )
	#define pinkySetCompare(_comp) TC_SetCompareB( &TCE0, _comp )
	
	/**
	 * Configuration directives
	 */
	// Those afferct the PWM frequency and resolution. See the XMEGA manual
	#define COMPARE_MAX    4999 // sets the servo frequency to 50Hz
	#define CLK_DIV        TC_CLKSEL_DIV4_gc
	// The servo position will be initialized to this value
	#define INIT_COMP      125
	// offset on the current reading
	#define CURRENT_OFFSET 6
	// offset on the angle measurement (degrees)
	#define ANGLE_OFFSET   32

	/**
	 * Initialize the hardware components needed for servo operation
	 */
	void servo_init();

	/**
	 * Set some angle to the desired servo.
	 * Angle must be between 0 and 180. Values outside such range will be
	 * cropped
	 */
	void servo_setAngle(uint8_t servo_num, uint16_t angle);
	
	/**
	 * Set the servo current
	 */
	void servo_setCurrent(uint8_t servo_num, uint8_t current);
#endif
