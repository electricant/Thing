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
	 * Various hardware assignments
	 */
	#define ESP_USART          USARTD0
	#define ESP_USART_DRE_vect USARTD0_DRE_vect
	#define ESP_USART_RXC_vect USARTD0_RXC_vect

	#define BATTERY_VOLTAGE_PIN ADC_CH_MUXPOS_PIN0_gc
	#define ADC_NEG_PIN         ADC_CH_MUXNEG_PIN4_gc
	/**
	 * Define where the servos are connected and their function
	 */
	#define THUMB_FINGER  0
	#define INDEX_FINGER  1
	#define MIDDLE_FINGER 2
	#define RING_FINGER   3
	#define PINKY_FINGER  4

	#define thumbSetCompare(_comp) TC_SetCompareA( &TCD0, _comp )
	#define indexSetCompare(_comp) TC_SetCompareB( &TCD0, _comp )
	#define middleSetCompare(_comp) TC_SetCompareC( &TCD0, _comp )
	#define ringSetCompare(_comp) TC_SetCompareA( &TCE0, _comp )
	#define pinkySetCompare(_comp) TC_SetCompareB( &TCE0, _comp )

	#define THUMB_CURRENT_PIN  ADC_CH_MUXPOS_PIN9_gc
	#define THUMB_ANGLE_PIN    ADC_CH_MUXPOS_PIN10_gc

	#define INDEX_CURRENT_PIN  ADC_CH_MUXPOS_PIN11_gc
	#define INDEX_ANGLE_PIN    ADC_CH_MUXPOS_PIN8_gc

	#define MIDDLE_CURRENT_PIN ADC_CH_MUXPOS_PIN7_gc
	#define MIDDLE_ANGLE_PIN   ADC_CH_MUXPOS_PIN6_gc

	#define RING_CURRENT_PIN   ADC_CH_MUXPOS_PIN5_gc
	#define RING_ANGLE_PIN     ADC_CH_MUXPOS_PIN3_gc

	#define PINKY_CURRENT_PIN  ADC_CH_MUXPOS_PIN1_gc
	#define PINKY_ANGLE_PIN    ADC_CH_MUXPOS_PIN0_gc

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
	union wifiCommand_le // Little-endian version
	{
		struct
		{
			uint8_t data;
			uint8_t command : 4;
			uint8_t servo : 4;
		}field;
		uint16_t raw;
	};
	#define WIFI_SET_MODE    0x01
	#define WIFI_SET_ANGLE   0x02
	#define WIFI_SET_CURRENT 0x03
	#define WIFI_SET_SPEED   0x04

	#define WIFI_GET_ANGLE   0x05
	#define WIFI_GET_CURRENT 0x06
	#define WIFI_GET_SPEED   0x07

	#define WIFI_MODE_FOLLOW 0x00
	#define WIFI_MODE_ANGLE  0x01
	#define WIFI_MODE_HOLD   0x02
#endif
