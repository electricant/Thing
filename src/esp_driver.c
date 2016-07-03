/**
 * 
 * 
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#include "include/esp_driver.h"
#include "include/serio_driver.h"

// Struct holding the command queue. Declared as volatile in order not to be
// optimized out by the compiler
struct CommandQueue {
	union wifiCommand cmd[COMMAND_QUEUE_SIZE];
	uint8_t next; // index of the next element in the array
	uint8_t nQueued; // nomber of enqueued items
};
static volatile struct CommandQueue cmdQ;

// parser status
static esp_state_t pStatus = BEGIN;

/**
 * Send a C string through the serial port
 */
void transmitStr(char* data)
{
	while (*data != 0)
	{
		while(!USART_IsTXDataRegisterEmpty(&USARTF0)) {;}
		USART_PutChar(&USARTF0, *data);
		++data;
	}
}

ISR(USARTF0_RXC_vect)
{
	static uint8_t skipCount; // number of characters to be skipped
	static uint8_t dataLen;   // length of the received packet

	char in = USART_GetChar(&USARTF0);

	switch (pStatus) {
		case BEGIN:
			if (in == '+') {
				skipCount = 6;  // when data is received the ESP sends:
				pStatus = SKIP_TO_LENGTH; // +IPD,0,n:<data>
			}	                // so skip to the number of bits n
			break;

		case SKIP_TO_LENGTH:
			skipCount--;
			if (skipCount == 0)
				pStatus = COMPUTE_LEN;
			break;

		case COMPUTE_LEN: // see 'man ascii' for details about conversion
			dataLen = min(in - 48, 8); // up to 8 bits at a time
			pStatus = SKIP_TO_DATA;
			break;

		case SKIP_TO_DATA:
			// just discard one char
			pStatus = FETCH_HIGH;
			break;

		case FETCH_HIGH:
			cmdQ.cmd[cmdQ.next].raw = in << 8; // set the high byte
			pStatus = FETCH_LOW;
			break;

		case FETCH_LOW:
			cmdQ.cmd[cmdQ.next].raw |= in; // set the low byte

			cmdQ.next = (cmdQ.next + 1) % COMMAND_QUEUE_SIZE;
			if (cmdQ.nQueued < COMMAND_QUEUE_SIZE)
				cmdQ.nQueued++;

			dataLen -= 2;
			if (dataLen == 0)
				pStatus = BEGIN;
			else
				pStatus = FETCH_HIGH;
			break;
	}
}

void esp_init()
{
	// initialize serial port
	PORTF.DIRSET = PIN3_bm; // PIN3 (TXD0) output
	PORTF.DIRCLR = PIN2_bm; // PIN2 (RXD0) input
	USART_Format_Set(&USARTF0, USART_CHSIZE_8BIT_gc, USART_PMODE_DISABLED_gc, 
			false);
	USART_Baudrate_Set(&USARTF0, 11, -7); // 115200 baud (see the XMEGA manual)
	USART_Rx_Enable(&USARTF0);
	USART_Tx_Enable(&USARTF0);
	USART_RxdInterruptLevel_Set(&USARTF0, USART_RXCINTLVL_LO_gc);

	// initialize ESP8266
	_delay_ms(1000); // wait device startup, just to be sure
	transmitStr("ATE0\r\n");
	_delay_ms(10);
	transmitStr("AT+CWMODE=2\r\n");
	_delay_ms(10);
	transmitStr("AT+CWSAP=\"Thing\",\"\",5,0\r\n");
	_delay_ms(10);
	transmitStr("AT+CIPMUX=1\r\n");
	_delay_ms(10);
	transmitStr("AT+CIPSTO=60\r\n"); // client activity timeout
	_delay_ms(10);
	transmitStr("AT+CIPSERVER=1\r\n"); // default port = 333

	// initialize an empty command Queue
	cmdQ.next = 0;
	cmdQ.nQueued = 0;
}

union wifiCommand esp_getCommand(bool blocking)
{
	// wait until there's at least one command stored in the queue
	while ((blocking == true) && (cmdQ.nQueued == 0)) {;}

	// dequeue the oldest element
	uint8_t index = mod(cmdQ.next - cmdQ.nQueued, COMMAND_QUEUE_SIZE);
	if (cmdQ.nQueued > 0)
		cmdQ.nQueued--;

	return cmdQ.cmd[index];
}

void esp_sendStr(char* string)
{
	transmitStr(string); // TODO
}
