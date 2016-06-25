#ifndef ESP_DRIVER_H
#define ESP_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

	/**
	 * Size of the receive buffer in bytes
	 */
	#define RECEIVE_BUFFER_SIZE 16
	/**
	 * String to send upon connection. Keep this length constant!
	 */
	#define CONNECT_STR "RA Thing v0.0\n"
	/**
	 * Status masks
	 */
	#define ESP_STATUS_READY     (0x01)
	#define ESP_STATUS_NEW_COMM  (0x01<<1)

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
	uint16_t esp_getCommand(bool blocking);

	/**
	 * Send a C string through the wifi link.
	 * If the link is not ready it will fail silently.
	 */
	void esp_sendStr(char* string);
#endif
