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
	/**
	 * Driver operating modes:
	 *
	 * ANGLE:  The servo tries to go to the desired angle with a given speed.
	 *         If a current threshold is crossed the driving signal is
	 *         momentarily disabled
	 *
	 * HOLD:   The servo tries to hold an object into the hand, applying the
	 *         desired force. This means that a given angle is kept as long as
	 *         the current stays below a threshold. If the threshold is crossed
	 *         the hand is opened slightly.
	 *
	 * FOLLOW: The driver does nothing. It sends pulses to the servo trying to
	 *         follow the position in which the operator is putting this hand's
	 *         fingers.
	 */
	typedef enum { ANGLE, HOLD, FOLLOW } servo_state_t;

	/**
	 * Configuration directives
	 */
	// Those afferct the PWM frequency and resolution. See the XMEGA manual
	#define COMPARE_MAX    4999 // sets the servo frequency to 50Hz
	#define CLK_DIV        TC_CLKSEL_DIV4_gc
	// Minimun output value for servo PWM
	#define SERVO_PWM_MIN  125
	// Maximum output value for servo PWM
	#define SERVO_PWM_MAX  625
	// Divider for the speed, used to slow down servo motion
	#define SPEED_DIVIDER  2
	// Default maximum current in mA
	#define DEF_CURRENT_MA 50
	// offset on the current reading
	#define CURRENT_OFFSET 6
	// offset on the angle measurement (degrees)
	#define ANGLE_OFFSET   32

	/**
	 * Initialize the hardware components needed for servo operation
	 */
	void servo_init();

	/**
	 * Choose the operating mode for this dirver
	 */
	void servo_setMode(const servo_state_t mode);

	/**
	 * Set some angle to the desired servo.
	 * Angle must be between 0 and 180. Values outside such range will be
	 * cropped.
	 *
	 * NOTE: Choosing an invalid servo number may result in erratic behaviour.
	 */
	void servo_setAngle(const uint8_t servo_num, const uint8_t angle);

	/**
	 * Set the servo current in milliamperes.
	 *
	 * NOTE: Choosing an invalid servo number may result in erratic behaviour.
	 */
	void servo_setCurrent(const uint8_t servo_num, const uint8_t current_mA);

	/**
	 * Set the speed at which te servo rotates when in ANGLE mode. This speed is
	 * a value which will be summed to the current angle until the goal is
	 * reached.
	 *
	 * NOTE: Choosing an invalid servo number may result in erratic behaviour.
	 */
	void servo_setSpeed(const uint8_t servo_num, const uint8_t speed);

	/**
	 * Return the current angle for the chosen servomotor
	 * 
	 * NOTE: Choosing an invalid servo number may result in erratic behaviour.
	 */
	uint8_t servo_getAngle(const uint8_t servo_num);

	/**
	 * Return the current sunk by the chosen servomotor
	 * 
	 * NOTE: Choosing an invalid servo number may result in erratic behaviour.
	 */
	uint8_t servo_getCurrent(const uint8_t servo_num);

	/**
	 * Return the current speed for the chosen servomotor
	 * 
	 * NOTE: Choosing an invalid servo number may result in erratic behaviour.
	 */
	uint8_t servo_getSpeed(const uint8_t servo_num);
#endif
