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

uint16_t thumbComp = SERVO_PWM_MIN;
uint16_t indexComp = SERVO_PWM_MIN;
uint16_t midComp = SERVO_PWM_MIN;
uint16_t ringComp = SERVO_PWM_MIN;
uint16_t pinkyComp = SERVO_PWM_MIN;

uint16_t thumbCurrent = 0;
uint16_t indexCurrent = 0;
uint16_t midCurrent = 0;
uint16_t ringCurrent = 0;
uint16_t pinkyCurrent = 0;

uint8_t tAngleReq = 0;
uint8_t tAngleAct = 0;
uint8_t tCurrentReq = 0;

void servo_init()
{
	// Setup the timer-counters for PWM operation
	PORTD.DIR = 0x01;
	TC_SetPeriod(&TCD0, COMPARE_MAX);
	TC0_ConfigWGM(&TCD0, TC_WGMODE_DS_T_gc);
	TC0_EnableCCChannels(&TCD0, TC0_CCAEN_bm);
	TC0_ConfigClockSource(&TCD0, CLK_DIV);
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
}

/**
 * Interrupt service routine. It transfers the requested servo angle to the PWM
 * subsystem, checking that the current does not exceed the threshold.
 *
 * TODO: maybe one for each servo group?
 */
ISR(TCD0_CCA_vect)
{
	if (thumbCurrent >= tCurrentReq) { // I ~= N / 2048
		if (tAngleAct > tAngleReq) // something is forcing us to 180 degrees
			thumbComp += 1;
		else
			thumbComp -= 1;
	} else {
		if (tAngleAct > (tAngleReq + 1))
			thumbComp -= 2;
		else if (tAngleAct < (tAngleReq - 1))
			thumbComp += 2;
	}

	thumbComp = max(thumbComp, SERVO_PWM_MIN);
	thumbComp = min(thumbComp, SERVO_PWM_MAX);
	thumbSetCompare(thumbComp);
	
	indexSetCompare(indexComp);
	
	middleSetCompare(midComp);
	
	ringSetCompare(ringComp);
	
	pinkySetCompare(pinkyComp);
}

ISR(ADCA_CH0_vect)
{
	// use a temporary value for calculations to avoid wrong readings
	int16_t tempC = ADC_ResultCh_GetWord_Signed(&ADCA.CH0, CURRENT_OFFSET);
	tempC = tempC / 2; // remove LSB to get 10bit resolution
	tempC += thumbCurrent; // smooth the result
	thumbCurrent = max(0, tempC / 2); // remove sign

	// select another servo

	// start another conversion
	ADC_Ch_Conversion_Start(&ADCA.CH0);
}

ISR(ADCA_CH1_vect)
{
	int16_t tempA = ADC_ResultCh_GetWord_Signed(&ADCA.CH1, 0);
	tempA = (tempA * 9) / 32;
	tempA = (tempA * 5) / 16;
	tempA -= ANGLE_OFFSET;
	tempA += tAngleAct;
	tAngleAct = max(0, tempA / 2); // remove sign

	// select another servo

	// start another conversion
	ADC_Ch_Conversion_Start(&ADCA.CH1);
}

void servo_setAngle(uint8_t servo_num, uint16_t angle)
{
	angle = max(angle, 1); // see how the control algorithm works
	angle = min(angle, 180);

	switch (servo_num)
	{
		case THUMB_FINGER:
			tAngleReq = angle;
			break;
		case INDEX_FINGER:
			
			break;
		case MIDDLE_FINGER:
			
			break;
		case RING_FINGER:
			
			break;
		case PINKY_FINGER:
			
			break;
	}
}

void servo_setCurrent(uint8_t servo_num, uint8_t current)
{
	tCurrentReq = current; // TODO: other fingers
}
