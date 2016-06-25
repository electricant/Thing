/**
 * Global configuration file for the board. All the compile-time options go here
 * 
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */

#ifndef BOARD_H
#define BOARD_H

	/**
	 * Clock configuration
	 */
	// CPU clock frequency. USed for timing and other stuff
	#define F_CPU           2000000UL
	// unset to use the external 16MHz crystal oscillator
	#define USE_INTERNAL_CLOCK

#endif
