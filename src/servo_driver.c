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

static servo_state_t status = FOLLOW; // start in a safe mode

struct servo_data_t
{
	uint16_t controlPWM; // Value to send to the CC module
	uint8_t  current_mA; // current measured by the ADC
	uint8_t  maxCurrent_mA; // maximum allowed current
	uint8_t  angle_deg; // angle measured by the ADC
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

	// Setup the ADC channel 0 to read currents
	ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH0, ADC_CH_INPUTMODE_DIFFWGAIN_gc,
			ADC_CH_GAIN_4X_gc);
	ADC_Ch_InputMux_Config(&ADCA.CH0, ADC_CH_MUXPOS_PIN0_gc,
			ADC_CH_MUXNEG_PIN4_gc);
	ADC_Ch_Interrupts_Config(&ADCA.CH0, ADC_CH_INTMODE_COMPLETE_gc,
			ADC_CH_INTLVL_MED_gc);
	ADC_Ch_Conversion_Start(&ADCA.CH0);

	// Setup ADC channel 1 to read angles
	ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH1, ADC_CH_INPUTMODE_DIFFWGAIN_gc,
			            ADC_CH_GAIN_1X_gc); // divide by two
	ADC_Ch_InputMux_Config(&ADCA.CH1, ADC_CH_MUXPOS_PIN1_gc,
	    		        ADC_CH_MUXNEG_PIN4_gc);
	ADC_Ch_Interrupts_Config(&ADCA.CH1, ADC_CH_INTMODE_COMPLETE_gc,
			            ADC_CH_INTLVL_MED_gc);
	ADC_Ch_Conversion_Start(&ADCA.CH1);

	// provide some safe default values
	for (int i = 0; i < 5; i++) {
		sData[i].controlPWM = SERVO_PWM_MIN;
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
		uint16_t targetComp = angle2comp(sData[i].targetAngle_deg);
		uint8_t speed = sData[i].speed;
		uint8_t actualAngle = sData[i].angle_deg;
		uint8_t actualCurrent = sData[i].current_mA;
		uint8_t maxCurrent = sData[i].maxCurrent_mA;

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
					compVal = max(compVal, SERVO_PWM_MIN);
				}
				break;

			case FOLLOW:
				if ((compVal == 0) && (actualCurrent < 5))
				{ // compVal was zero so the current is negligible and the angle
				  // reading correct
					compVal = angle2comp(actualAngle);
				} else {
					compVal = 0;
				}
				break;
		}

		if (compVal >= SERVO_PWM_MIN) // save only if valid
			sData[i].controlPWM = compVal;

		// set the capture-compare value to the correct servo
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

ISR(ADCA_CH0_vect)
{
	static int8_t servo_num = 0;
	// use a temporary value for calculations to avoid wrong readings
	int16_t tempC = ADC_ResultCh_GetWord_Signed(&ADCA.CH0, CURRENT_OFFSET);
	// remove the sign, thus leaving 11 bits resolution
	tempC = max(0, tempC);
	// convert the reading int a current by multiplying by 1000/4096
	tempC = (tempC * 25) / 32;
	tempC = (tempC * 5) / 16;
	// smooth the result and save it
	tempC += sData[servo_num].current_mA;
	sData[servo_num].current_mA = tempC / 2;

	// select another servo
	//servo_num = (servo_num + 1) % 5;

	// start another conversion
	// TODO: do I have to flush the ADC pipeline?
	ADC_Ch_Conversion_Start(&ADCA.CH0);
}

ISR(ADCA_CH1_vect)
{
	static int8_t servo_num = 0;
	// temporary value for angle readings (TODO: single ended)
	uint16_t tempA = max(0, ADCA.CH1.RES);
	// multiply by 180/2048 to obtain an angle
	tempA = (tempA * 9) / 32;
	tempA = (tempA * 5) / 16;
	// subtract an offset due to the servo potentiometer
	tempA -= ANGLE_OFFSET;
	// smooth and save
	tempA += sData[servo_num].angle_deg;
	sData[servo_num].angle_deg = tempA / 2;

	// select another servo
	//servo_num = (servo_num + 1) % 5;

	// start another conversion
	ADC_Ch_Conversion_Start(&ADCA.CH1);
}

void servo_setMode(const servo_state_t mode)
{
	status = mode;
}

void servo_setAngle(const uint8_t servo_num, uint16_t angle)
{
	angle = max(angle, 1); // setting angle to 0 may screw the control
	                       // algorithm
	angle = min(angle, 180);

	sData[servo_num].targetAngle_deg = angle;
}

void servo_setCurrent(const uint8_t servo_num, const uint8_t current_mA)
{
	sData[servo_num].maxCurrent_mA = current_mA;
}

void servo_setSpeed(const uint8_t servo_num, const uint8_t speed)
{
	sData[servo_num].speed = speed;
}

