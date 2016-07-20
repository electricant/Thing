/**
 * This driver is responsible for controlling the ADC and providing the various
 * readings to the other modules.
 *
 * It is based on prrevious work by Atmel Corporation (http://www.atmel.com)
 * and it has been adapted to suit this application
 *
 * Copyright (C) 2008 Atmel Corporation <avr@atmel.com>
 * Copyright (C) 2016 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#include "include/adc_driver.h"
#include "include/utils.h"
#include "include/serio_driver.h"

/*
 * The ADC is continuously running. The various channels are scanned one at a
 * time. This data structure keeps track of the conversion.
 *
 * 11 different conversions are needed because we have to mesaure angle and
 * current of 5 servos plus the battery voltage
 */
static volatile struct ADC_Conversion_t conv[11];
static volatile uint8_t convIndex = 0;

void ADC_init()
{
	ADC_ConvMode_and_Resolution_Config(&ADCA, ADC_ConvMode_Signed,
			ADC_RESOLUTION_12BIT_gc);
	ADC_Reference_Config(&ADCA, ADC_REFSEL_INT1V_gc);
	ADC_Prescaler_Config(&ADCA, ADC_PRESCALER_DIV128_gc); // f_samp = 5682Hz

	ADC_Ch_Interrupts_Config(&ADCA.CH0, ADC_CH_INTMODE_COMPLETE_gc,
			ADC_CH_INTLVL_MED_gc);

	ADC_Enable(&ADCA);
	ADC_waitSettle(&ADCA);

	// initialize the conversion struct
	conv[THUMB_FINGER].muxposPin = THUMB_CURRENT_PIN;
	conv[THUMB_FINGER].gain = CURRENT_GAIN;
	conv[THUMB_FINGER + 1].muxposPin = THUMB_ANGLE_PIN;
	conv[THUMB_FINGER + 1].gain = ANGLE_GAIN;

	conv[INDEX_FINGER * 2].muxposPin = INDEX_CURRENT_PIN;
	conv[INDEX_FINGER * 2].gain = CURRENT_GAIN;
	conv[(INDEX_FINGER * 2) + 1].muxposPin = INDEX_ANGLE_PIN;
	conv[(INDEX_FINGER * 2) + 1].gain = ANGLE_GAIN;

	conv[MIDDLE_FINGER * 2].muxposPin = MIDDLE_CURRENT_PIN;
	conv[MIDDLE_FINGER * 2].gain = CURRENT_GAIN;
	conv[(MIDDLE_FINGER * 2) + 1].muxposPin = MIDDLE_ANGLE_PIN;
	conv[(MIDDLE_FINGER * 2) + 1].gain = ANGLE_GAIN;

	conv[RING_FINGER * 2].muxposPin = RING_CURRENT_PIN;
	conv[RING_FINGER * 2].gain = CURRENT_GAIN;
	conv[(RING_FINGER * 2) + 1].muxposPin = RING_ANGLE_PIN;
	conv[(RING_FINGER * 2) + 1].gain = ANGLE_GAIN;

	conv[PINKY_FINGER * 2].muxposPin = PINKY_CURRENT_PIN;
	conv[PINKY_FINGER * 2].gain = CURRENT_GAIN;
	conv[(PINKY_FINGER * 2) + 1].muxposPin = PINKY_ANGLE_PIN;
	conv[(PINKY_FINGER * 2) + 1].gain = ANGLE_GAIN;

	// battery voltage
	conv[10].gain = BATTERY_GAIN;
	conv[10].muxposPin = ADC_CH_MUXPOS_PIN2_gc;

	ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH0, ADC_CH_INPUTMODE_DIFFWGAIN_gc,
			CURRENT_GAIN);
	ADC_Ch_InputMux_Config(&ADCA.CH0, INDEX_CURRENT_PIN, ADC_NEG_PIN);
	ADC_Ch_Conversion_Start(&ADCA.CH0);
}

ISR(ADCA_CH0_vect)
{
	ADCA.CH0.INTFLAGS = ADC_CH_CHIF_bm; // clear interrupt flag
	// I'm not interested in the sign of the data
	int16_t curRes = ADCA.CH0RES;

	conv[convIndex].result = max(0, curRes);

	// prepare the ADC for the next reading
	convIndex = (convIndex + 1) % 11;

	ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH0, ADC_CH_INPUTMODE_DIFFWGAIN_gc,
			conv[convIndex].gain);
	ADC_Ch_InputMux_Config(&ADCA.CH0, conv[convIndex].muxposPin, ADC_NEG_PIN);
	ADC_Pipeline_Flush(&ADCA);
	ADC_Ch_Conversion_Start(&ADCA.CH0);
}

inline uint8_t ADC_getServoCurrent(uint8_t servo_num)
{
	uint16_t tempC = conv[servo_num * 2].result;
	//tempC = (tempC * 3125) / 3072; // same as 1 (1% rounding error)
	tempC = min(255, tempC);
	return (tempC - CURRENT_OFFSET);
}

inline uint8_t ADC_getServoAngle(uint8_t servo_num)
{
	uint16_t tempA = conv[(servo_num * 2) + 1].result;
	tempA = (tempA * 9) / 32;
	tempA = (tempA * 5) / 16;
	return tempA - ANGLE_OFFSET;
}

inline uint8_t ADC_getBatteryVoltage()
{
	return (conv[10].result >> 3); // the sign has no meaning
}

/* Prototype for assembly macro. */
uint8_t SP_ReadCalibrationByte( uint8_t index );

void ADC_loadCalibrationValues(ADC_t * adc)
{
	if (&ADCA == adc) {
		/* Get ADCACAL0 from production signature . */
		adc->CALL = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL0_offset );
		adc->CALH = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL1_offset );
	} else {
		/* Get ADCBCAL0 from production signature  */
		adc->CALL = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCBCAL0_offset );
		adc->CALH = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCBCAL1_offset );
	}
}

void ADC_waitSettle(ADC_t * adc)
{
	/* Store old prescaler value. */
	uint8_t prescaler_val = adc->PRESCALER;

	/* Set prescaler value to minimum value. */
	adc->PRESCALER = ADC_PRESCALER_DIV8_gc;

	/* wait 8*COMMON_MODE_CYCLES for common mode to settle*/
	delay_us(8*COMMON_MODE_CYCLES);

	/* Set prescaler to old value*/
	adc->PRESCALER = prescaler_val;
}

#ifdef __GNUC__

/*! \brief Function for GCC to read out calibration byte.
 *
 *  \note For IAR support, include the adc_driver_asm.S90 file in your project.
 *
 *  \param index The index to the calibration byte.
 *
 *  \return Calibration byte.
 */
uint8_t SP_ReadCalibrationByte( uint8_t index )
{
	uint8_t result;

	/* Load the NVM Command register to read the calibration row. */
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	result = pgm_read_byte(index);

	/* Clean up NVM Command register. */
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;

	return result;
}

#endif
