#ifndef ESP_DRIVER_H
#define ESP_DRIVER_H

	#include <stdbool.h>
	#include <stdint.h>

	#include "include/board.h"
	#include "include/usart_driver.h"
	#include "include/utils.h"

	/**
	 * Size of the command queue in number of commands. Old commands will be
	 * overwritten.
	 */
	#define COMMAND_QUEUE_SIZE 8

	/**
	 * The command parser is a state machine. This enum defines its states
	 */
	typedef enum {
		BEGIN,       // receive data and analyze its value
		SKIP_TO_LENGTH, // receive data ignoring its value
		COMPUTE_LEN, // compute the length of received data
		SKIP_TO_DATA,
		FETCH_HIGH,  // Fetch the high byte of the command
		FETCH_LOW    // Fetch the low byte of the command
	} esp_state_t;

	/**
	 * Initialize the hardware components needed for the WiFi module
	 */
	void esp_init();

	/**
	 * Get the last command issued.
	 *
	 * If blocking is set to true this routine will block and wait until a new
	 * command is received.
	 * If blocking is set to false the routine does not block but my return an
	 * old command.
	 */
	union wifiCommand esp_getCommand(const bool blocking);

	/**
	 * Send a command through the wifi link.
	 * If the link is not ready it will fail silently.
	 */
	void esp_sendCommand(const union wifiCommand cmd);
#endif
