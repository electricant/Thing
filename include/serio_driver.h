#ifndef SERIO_DRIVER_H
#define SERIO_DRIVER_H


	/**
	 * Size of the transmitt an send buffer in bytes
	 */
	#define BUFFER_SIZE 16

	/**
	 *
	 */
	void serio_init();

	/**
	 * Send a single character through the serial port
	 */
	void serio_putChar(char c);

	/**
	 * Send a C string through the serial port
	 */
	void serio_putString(char* string);

	/**
	 * Write len bytes read from buf to the serial port
	 */
	void serio_writeBuffer(uint8_t* buf, size_t len);
#endif
