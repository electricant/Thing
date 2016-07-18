/**
 * Implementation for battery_driver.h
 *
 * Copyright (C) 2016 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#include "include/board.h"
#include "include/avr_compiler.h"
#include "include/battery_driver.h"
#include "include/adc_driver.h"
#include "include/TC_driver.h"

void battery_init()
{
	PORTD.DIRSET = PIN4_bm | PIN5_bm | PIN6_bm; // battery level indicator
	PORTD.OUT = 0x00;

	PORTC.DIRSET = PIN7_bm; // signal to stop the boost converters
	PORTC.DIRCLR = PIN6_bm; // input V_USB_CHG
	PORTC.OUT = 0x00;

	PORTE.DIRCLR = PIN3_bm; // input CHG_STAT

	// update once for each second (1Hz)
	TC_SetPeriod(&TCE0, 15625);
	//TC_SetCompareA( &TCE0, 7812);
	TC0_ConfigClockSource(&TCE0, TC_CLKSEL_DIV64_gc);
	TC0_ConfigWGM(&TCE0, TC_WGMODE_NORMAL_gc);
	TC0_EnableCCChannels(&TCE0, TC0_CCAEN_bm);
	TC0_SetCCAIntLevel(&TCE0, TC_CCAINTLVL_LO_gc);
}

ISR(TCE0_CCA_vect)
{
	uint8_t volt = ADC_getBatteryVoltage();
	static uint8_t blink_state = 0;

	if (volt >= 220) { // HIGH
		PORTD.OUT = 0x70; // all LEDs on
		PORTC.OUTCLR = PIN7_bm;
	} else if (volt >= 190) { // MED
		PORTD.OUT = 0x30;
		PORTC.OUTCLR = PIN7_bm;
	} else if (volt >= 161) { // LOW
		PORTD.OUT = 0x10;
		PORTC.OUTCLR = PIN7_bm;
	} else { // STOP everything
		PORTC.OUTSET = PIN7_bm; // stop the converters

		if (PORTD.OUT == 0x00) // toggle battery LEDs
			PORTD.OUT = ~PORTD.OUT;
		else
			PORTD.OUT = 0x00;
	}
	
	if (hasExternalPower()) {
		PORTC.OUTSET = PIN7_bm; // turn off the converters
		
		if (!chargeComplete() && (blink_state == 0)) {
			// charging -> toggle LEDs, starting from the current voltage
			PORTD.OUT = 0x00;
			blink_state = 1;
		} else {
			blink_state = 0;
		}
	}
}
