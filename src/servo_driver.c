/**
 *
 *
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#include "include/board.h"
#include "include/avr_compiler.h"
#include "include/TC_driver.h"
#include "include/adc_driver.h"

#include "include/servo_driver.h"
#include "include/utils.h"

static volatile servo_state_t status = FOLLOW; // start in a safe mode

struct servo_data_t
{
	uint16_t controlPWM; // Value to send to the CC module, multiplied by
	                     // SPEED_DIVIDER to get fractional speed
	uint8_t  maxCurrent_mA; // maximum allowed current
	uint8_t  targetAngle_deg; // angle to be reached
	uint8_t  speed; // speed of angle rotation
};
static struct servo_data_t sData[5];

void servo_init()
{
	// Setup the timer-counters for PWM operation
	PORTD.DIR = 0x01;
	TC_SetPeriod(&TCD0, COMPARE_MAX);
	TC0_ConfigClockSource(&TCD0, CLK_DIV);
	TC0_ConfigWGM(&TCD0, TC_WGMODE_DS_T_gc);
	TC0_EnableCCChannels(&TCD0, TC0_CCAEN_bm);
	TC0_SetCCAIntLevel(&TCD0, TC_CCAINTLVL_MED_gc);

	// provide some safe default values
	for (int i = 0; i < 5; i++) {
		sData[i].controlPWM = SERVO_PWM_MIN * SPEED_DIVIDER;
		sData[i].targetAngle_deg = 0;
		sData[i].maxCurrent_mA = DEF_CURRENT_MA;
		sData[i].speed = 1;
	}
}
/**
 * Convert an angle in degrees into a valid capture-compare value
 */
inline uint16_t angle2comp(uint8_t angle)
{
	return SERVO_PWM_MIN + (angle * 25) / 9; // (MAX-MIN) / 180
}

/**
 * Interrupt service routine. It transfers the requested servo angle to the PWM
 * subsystem, checking that the current does not exceed the threshold.
 *
 * TODO: maybe one for each servo group?
 */
ISR(TCD0_CCA_vect)
{
	for (int i = 0; i < 5; i++)
	{ // update the driving signal for each servo
		uint16_t compVal = sData[i].controlPWM;
		uint16_t targetComp = angle2comp(sData[i].targetAngle_deg)
			* SPEED_DIVIDER;
		uint8_t speed = sData[i].speed;
		uint8_t maxCurrent = sData[i].maxCurrent_mA;
		uint8_t actualAngle = ADC_getServoAngle(i);
		uint8_t actualCurrent = ADC_getServoCurrent(i);

		switch (status)
		{
			case ANGLE:
				if (actualCurrent < maxCurrent) {
					if (compVal < targetComp) {
						compVal += speed;
						compVal = min(compVal, targetComp);
					} else if (compVal > targetComp) {
						compVal -= speed;
						compVal = max(compVal, targetComp);
					}
				} else {
					compVal = 0; // too much current. STOP!
				}
				break;

			case HOLD:
				// NOTE: I'm supposing the hand is closed when the servo goes
				// to 180 degrees and opened otherwhise
				if (actualCurrent < maxCurrent) {
					compVal++; // hold it slowly, yum!
					compVal = min(compVal, targetComp);
				} else {
					compVal -= 10;
					compVal = max(compVal, SERVO_PWM_MIN * SPEED_DIVIDER);
				}
				break;

			case FOLLOW:
				if ((compVal == 0) && (actualCurrent < 5))
				{ // compVal was zero so the current is negligible and the angle
				  // reading correct
					compVal = angle2comp(actualAngle) * SPEED_DIVIDER;
				} else {
					compVal = 0;
				}
				break;
		}

		if (compVal >= SERVO_PWM_MIN * SPEED_DIVIDER) // save only if valid
			sData[i].controlPWM = compVal;

		// set the capture-compare value to the correct servo
		compVal = compVal / SPEED_DIVIDER;
		switch (i) {
			case THUMB_FINGER:
				thumbSetCompare(compVal);
				break;

			case INDEX_FINGER:
				indexSetCompare(compVal);
				break;

			case MIDDLE_FINGER:
				middleSetCompare(compVal);
				break;

			case RING_FINGER:
				ringSetCompare(compVal);
				break;

			case PINKY_FINGER:
				pinkySetCompare(compVal);
				break;
		}
	}
}

void servo_setMode(const servo_state_t mode)
{
	status = mode;
}

void servo_setAngle(const uint8_t servo_num, const uint8_t angle)
{
	uint8_t a = max(angle, 1); // setting angle to 0 may screw the control
	                           // algorithm
	a = min(a, 180);

	sData[servo_num].targetAngle_deg = a;
}

void servo_setCurrent(const uint8_t servo_num, const uint8_t current_mA)
{
	sData[servo_num].maxCurrent_mA = current_mA;
}

void servo_setSpeed(const uint8_t servo_num, const uint8_t speed)
{
	sData[servo_num].speed = speed;
}

uint8_t servo_getAngle(const uint8_t servo_num)
{
	uint16_t actPWM = sData[servo_num].controlPWM / SPEED_DIVIDER;
	actPWM -= SERVO_PWM_MIN;
	return (actPWM * 9) / 25;
}

uint8_t servo_getSpeed(const uint8_t servo_num)
{
	uint8_t current = ADC_getServoCurrent(servo_num);

	if ((current > 10) && (current < sData[servo_num].maxCurrent_mA))
		return sData[servo_num].speed;

	return 0;
}
