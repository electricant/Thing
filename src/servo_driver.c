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

struct servo_data_t
{
	uint16_t controlPWM; // Value to send to the CC module
	uint8_t  current_mA; // current measured by the ADC
	uint8_t  maxCurrent_mA; // maximum allowed current
	uint8_t  angle_deg; // angle measured by the ADC
	uint8_t  targetAngle_deg; // angle to be reached
}sData[5];

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
			ADC_CH_INTLVL_LO_gc);
	ADC_Ch_Conversion_Start(&ADCA.CH0);

	// Setup ADC channel 1 to read angles
	ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH1, ADC_CH_INPUTMODE_DIFFWGAIN_gc,
			            ADC_CH_GAIN_1X_gc); // divide by two
	ADC_Ch_InputMux_Config(&ADCA.CH1, ADC_CH_MUXPOS_PIN1_gc,
	    		        ADC_CH_MUXNEG_PIN4_gc);
	ADC_Ch_Interrupts_Config(&ADCA.CH1, ADC_CH_INTMODE_COMPLETE_gc,
			            ADC_CH_INTLVL_LO_gc);
	ADC_Ch_Conversion_Start(&ADCA.CH1);

	// provide some safe default values
	for (int i = 0; i < 5; i++) {
		sData[i].controlPWM = SERVO_PWM_MIN;
		sData[i].targetAngle_deg = 0;
		sData[i].maxCurrent_mA = DEF_CURRENT_MA;
	}
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
		uint8_t actualCurrent = sData[i].current_mA;
		uint8_t maxCurrent = sData[i].maxCurrent_mA;
		uint8_t actualAngle = sData[i].angle_deg;
		uint8_t targetAngle = sData[i].targetAngle_deg;

		if (actualCurrent > maxCurrent) {
			if (actualAngle > targetAngle)
			{ // something is forcing us to 180 degrees. Follow it
				compVal += 1;
			} else if (actualAngle < targetAngle) {
				compVal -= 1; // go to 0 degrees
			}
		} else {
			if (actualAngle > (targetAngle + 1))
				compVal -= 2;
			else if (actualAngle < (targetAngle - 1))
				compVal += 2;
		}

		compVal = max(compVal, SERVO_PWM_MIN);
		compVal = min(compVal, SERVO_PWM_MAX);
		sData[i].controlPWM = compVal;
	}

	thumbSetCompare(sData[THUMB_FINGER].controlPWM);
	indexSetCompare(sData[INDEX_FINGER].controlPWM);
	middleSetCompare(sData[MIDDLE_FINGER].controlPWM);
	ringSetCompare(sData[RING_FINGER].controlPWM);
	pinkySetCompare(sData[PINKY_FINGER].controlPWM);
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
	sData[servo_num].current_mA = (tempC + sData[servo_num].current_mA) / 2;

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
	sData[servo_num].angle_deg = (tempA + sData[servo_num].angle_deg) / 2;

	// select another servo
	//servo_num = (servo_num + 1) % 5;

	// start another conversion
	ADC_Ch_Conversion_Start(&ADCA.CH1);
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
