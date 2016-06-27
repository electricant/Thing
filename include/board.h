/**
 * Global configuration file for the board. All the compile-time options go here
 * 
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */

#ifndef BOARD_H
#define BOARD_H

	#include <stdint.h>

	/**
	 * Clock configuration
	 */
	// CPU clock frequency. USed for timing and other stuff
	#define F_CPU           2000000UL
	// unset to use the external 16MHz crystal oscillator
	#define USE_INTERNAL_CLOCK

	/**
	 * Commands supported through the wifi link
	 */
	union wifiCommand
	{
		struct
		{
			uint8_t command : 4;
			uint8_t servo : 4;
			uint8_t data;
		}field;
		uint16_t raw;
	};
	#define WIFI_SET_MODE    0x01
	#define WIFI_SET_ANGLE   0x02
	#define WIFI_SET_CURRENT 0x03
	#define WIFI_SET_SPEED   0x04

	#define WIFI_MODE_FOLLOW 0x00
	#define WIFI_MODE_ANGLE  0x01
	#define WIFI_MODE_HOLD   0x02
#endif
