/**
 *
 *
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#include "include/board.h"
#include "include/avr_compiler.h"
//#include "include/pmic_driver.h"
#include "include/clksys_driver.h"

#include "include/adc_driver.h"
#include "include/esp_driver.h"
#include "include/serio_driver.h"
#include "include/servo_driver.h"
#include "include/battery_driver.h"

/**
 * Firmware entry point
 */
int main( void )
{
	/*
	 * HARDWARE INITIALIZATION
	 */
	CLKSYS_XOSC_Config(OSC_FRQRANGE_12TO16_gc, false,
			OSC_XOSCSEL_XTAL_256CLK_gc);
	CLKSYS_Enable(OSC_XOSCEN_bm);
	do {} while ( CLKSYS_IsReady(OSC_XOSCRDY_bm) == 0);
	CLKSYS_Main_ClockSource_Select(CLK_SCLKSEL_XOSC_gc);

	ADC_init();
	servo_init();
	battery_init();
	serio_init();
	esp_init();

	// Enable all interrupts
	PMIC.CTRL = PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
	sei();

	serio_putString("\r\nRA Thing v0.0 READY\r\n> "); // TODO make it a prompt
	/*
	 * main loop: listens for comands and executes them
	 */
	while (1) {
		union wifiCommand cmd = esp_getCommand(true);

		serio_putChar('C');
		serio_putChar(cmd.field.command + 48);

		serio_putString(" S");
		serio_putChar(cmd.field.servo + 48);

		serio_putString(" D");
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
				cmd.field.data = ADC_getServoAngle(cmd.field.servo);
				esp_sendCommand(cmd);
				break;

			case WIFI_GET_CURRENT:
				cmd.field.data = ADC_getServoCurrent(cmd.field.servo);
				esp_sendCommand(cmd);
				break;

			case WIFI_GET_SPEED:
				cmd.field.data = servo_getSpeed(cmd.field.servo);
				esp_sendCommand(cmd);
				break;
		}
	}
}
