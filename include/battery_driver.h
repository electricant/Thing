/**
 * This driver creates a task that continuously monitors the battery
 * voltage and decides wether to stop the servo signals and the boost
 * converters.
 *
 * Copyright (C) 2016 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */

#ifndef BATTERY_DRIVER_H
#define BATTERY_DRIVER_H

#define chargeComplete()   (PORTE.IN & 0x08)
#define hasExternalPower() (PORTC.IN & 0x40)

/**
 * Initialize the battery driver and create the task
 */
void battery_init();

#endif
