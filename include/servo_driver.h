/**
 *
 *
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

	#include <stdint.h>

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
	#define COMPARE_MAX    19999 // sets the servo frequency to 50Hz
	#define CLK_DIV        TC_CLKSEL_DIV8_gc
	// Minimun output value for servo PWM
	#define SERVO_PWM_MIN  500
	// Maximum output value for servo PWM
	#define SERVO_PWM_MAX  2500
	// Divider for the speed, used to slow down servo motion
	#define SPEED_DIVIDER  2
	// Default maximum current in mA
	#define DEF_CURRENT_MA 250

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
	 * This function fails silently if servo_num is not valid
	 */
	void servo_setAngle(const uint8_t servo_num, const uint8_t angle);

	/**
	 * Set the servo current in milliamperes.
	 * This function fails silently if servo_num is not valid
	 */
	void servo_setCurrent(const uint8_t servo_num, const uint8_t current_mA);

	/**
	 * Set the speed at which te servo rotates when in ANGLE mode. This speed is
	 * a value which will be summed to the current angle until the goal is
	 * reached.
	 * This function fails silently if servo_num is not valid
	 */
	void servo_setSpeed(const uint8_t servo_num, const uint8_t speed);

	/**
	 * Return the current speed for the chosen servomotor
	 *
	 * NOTE: Choosing an invalid servo number may result in erratic behaviour.
	 */
	uint8_t servo_getSpeed(const uint8_t servo_num);
	/**
	 * Return the current angle for the chosen servomotor
	 *
	 * NOTE: Choosing an invalid servo number may result in erratic behaviour.
	 */
	uint8_t servo_getAngle(const uint8_t servo_num);
#endif
