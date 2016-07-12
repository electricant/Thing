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

	serio_putString("\r\nRA Thing v0.0 READY\r\n> "); // TODO make it a prompt
	/*
	 * main loop: listens for comands and executes them
	 */
	while (1) {
		union wifiCommand cmd = esp_getCommand(true);

		serio_putChar('C');
		serio_putChar(cmd.field.command + 48);
		serio_putChar(' ');

		serio_putChar('S');
		serio_putChar(cmd.field.servo + 48);
		serio_putChar(' ');

		serio_putChar('D');
		serio_putChar(cmd.field.data);
		serio_putString("\r\n");

		switch (cmd.field.command)
		{
			case WIFI_SET_MODE:
				if (cmd.field.data == WIFI_MODE_FOLLOW)
					servo_setMode(FOLLOW);
				else if (cmd.field.data == WIFI_MODE_ANGLE)
					servo_setMode(ANGLE);
				else if (cmd.field.data == WIFI_MODE_HOLD)
					servo_setMode(HOLD);
				break;

			case WIFI_SET_ANGLE:
				servo_setAngle(cmd.field.servo, cmd.field.data);
				break;

			case WIFI_SET_CURRENT:
				servo_setCurrent(cmd.field.servo, cmd.field.data);
				break;

			case WIFI_SET_SPEED:
				servo_setSpeed(cmd.field.servo, cmd.field.data);
				break;

			case WIFI_GET_ANGLE: // command is the same, with payload
				cmd.field.data = servo_getAngle(cmd.field.servo);
				esp_sendCommand(cmd);
				break;

			case WIFI_GET_CURRENT:
				cmd.field.data = servo_getCurrent(cmd.field.servo);
				esp_sendCommand(cmd);
				break;

			case WIFI_GET_SPEED:
				cmd.field.data = servo_getSpeed(cmd.field.servo);
				esp_sendCommand(cmd);
				break;
		}
	}
}
