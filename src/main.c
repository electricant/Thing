/**
 * 
 * 
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */

// Standard library

// Local drivers
#include "include/board.h"
#include "include/avr_compiler.h"
#include "include/pmic_driver.h"
#include "include/adc_driver.h"

#include "include/esp_driver.h"
#include "include/serio_driver.h"
#include "include/servo_driver.h"

/**
 * Firmware entry point
 */
int main( void )
{
	/*
	 * HARDWARE INITIALIZATION
	 *
	 * All the shared modules are initialized here. The rest is done within each
	 * driver.
	 */
	//TODO: clock_init();
	// Initialize LED output (xplained board)
	PORTE.DIR = 0xff;
	PORTE.OUT = 0xFF;
	
	// The ADC is needed for both the servo driver and the battery driver
	ADC_CalibrationValues_Load(&ADCA);
	ADC_ConvMode_and_Resolution_Config(&ADCA, ADC_ConvMode_Signed,
			ADC_RESOLUTION_12BIT_gc);
	ADC_Reference_Config(&ADCA, ADC_REFSEL_INT1V_gc);
	ADC_Prescaler_Config(&ADCA, ADC_PRESCALER_DIV256_gc); // TODO: f_samp = ?
	ADC_Enable(&ADCA);

	// Initialize specific drivers
	servo_init();
	serio_init();
	esp_init();
	
	// Interrupts are used pretty much everywhere
	PMIC_SetVectorLocationToApplication();
	PMIC_EnableLowLevel();
	PMIC_EnableMediumLevel();
	PMIC_EnableHighLevel();
	sei();

	/*
	 * Actual main
	 */
	uint8_t angle = 5;
	uint8_t current = 16;
	servo_setAngle(THUMB_FINGER, angle);
	servo_setCurrent(THUMB_FINGER, current);
	// test servo_driver
	while (1) {
		uint8_t angleCmd = esp_getCommand(false);
		uint8_t currentCmd = esp_getCommand(false)>>8;

		if (angleCmd < 37)
			angleCmd = 37;

		if (currentCmd < 42)
			currentCmd = 42;

		angle = angleCmd - 32;
		current = currentCmd - 32;
		servo_setAngle(THUMB_FINGER, angle);
		servo_setCurrent(THUMB_FINGER, current);
		_delay_ms(40);
	}
}
