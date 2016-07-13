/**
 * This driver is responsible for controlling the ESP8266 module and for command
 * transmission and reception.
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
static volatile struct CommandQueue txCmds, rxCmds;

// parser status
static esp_state_t pStatus = BEGIN;

// helper variables for command transmission
const static char* CMD_SEND = "AT+CIPSEND=0,2\r\n";

struct SendFlags {
	bool connected : 1;   // the client is connected
	bool initialized : 1; // the channel is ready to send data
	bool canSend     : 1; // the esp told us it is ready
};
static volatile struct SendFlags txFlags;
static volatile uint8_t txCount;

/**
 * Send a C string through the serial port
 */
void transmitStr(char* data);

/**
 * Initialization routine
 */
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
	USART_RxdInterruptLevel_Set(&USARTF0, USART_RXCINTLVL_MED_gc);

	// initialize ESP8266
	_delay_ms(1000); // wait device startup, just to be sure
	//transmitStr("ATE0\r\n");
	_delay_ms(10);
	transmitStr("AT+CWMODE=2\r\n");
	_delay_ms(10);
	transmitStr("AT+CWSAP=\"Thing\",\"\",5,0\r\n");
	_delay_ms(10);
	transmitStr("AT+CIPMUX=1\r\n");
	_delay_ms(10);
	transmitStr("AT+CIPSERVER=1\r\n"); // default port = 333
	_delay_ms(10);
	transmitStr("AT+CIPSTO=60\r\n"); // client activity timeout

	// initialize an empty command Queue
	txCmds.next = 0;
	txCmds.nQueued = 0;
	rxCmds.next = 0;
	rxCmds.nQueued = 0;

	// default flags
	txFlags.connected = false;
	txFlags.initialized = false;
	txFlags.canSend = false;
}

ISR(ESP_USART_RXC_vect)
{
	static uint8_t skipCount; // number of characters to be skipped
	static uint8_t dataLen;   // length of the received packet

	char in = USART_GetChar(&ESP_USART);

	switch (pStatus) {
		case BEGIN:
			if (in == '+') {
				skipCount = 6;  // when data is received the ESP sends:
				pStatus = SKIP_TO_LENGTH; // +IPD,0,n:<data>
				                // so skip to the number of bits n
			} else if (in == '0') {
				txFlags.connected = !txFlags.connected; // toggle
				txFlags.initialized = false;
				txCount = 0;
			} else if (in == '>') {
				txFlags.canSend = true;
				txCount = 0;
			}
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
			rxCmds.cmd[rxCmds.next].raw = in << 8; // set the high byte
			pStatus = FETCH_LOW;
			break;

		case FETCH_LOW:
			rxCmds.cmd[rxCmds.next].raw |= in; // set the low byte

			rxCmds.next = (rxCmds.next + 1) % COMMAND_QUEUE_SIZE;
			if (rxCmds.nQueued < COMMAND_QUEUE_SIZE)
				rxCmds.nQueued++;

			dataLen -= 2;
			if (dataLen == 0)
				pStatus = BEGIN;
			else
				pStatus = FETCH_HIGH;
			break;
	}
}

ISR(ESP_USART_DRE_vect)
{
	if (txCmds.nQueued == 0) {
		USART_DreInterruptLevel_Set(&ESP_USART, USART_DREINTLVL_OFF_gc);
	} else if (txFlags.connected == true) {
		if (txFlags.initialized == false) {
			char ch = *(CMD_SEND + txCount);
			
			if (ch != 0) {
				ESP_USART.DATA = ch;
				++txCount;
			} else { // string ended
				txFlags.initialized = true;
			}
		} else if (txFlags.canSend == true) {
			uint8_t index = mod(txCmds.next - txCmds.nQueued, 
					COMMAND_QUEUE_SIZE);
			char* cmd_ptr = (char*) &txCmds.cmd[index];
			char ch = *(cmd_ptr + txCount);
			ESP_USART.DATA = ch;

			txCount = (txCount + 1) % 2;
			if (txCount == 0) {
				--txCmds.nQueued;
			}
		}
	}
}

void transmitStr(char* data)
{
	while (*data != 0)
	{
		while(!USART_IsTXDataRegisterEmpty(&USARTF0)) {;}
		USART_PutChar(&USARTF0, *data);
		++data;
	}
}

union wifiCommand esp_getCommand(const bool blocking)
{
	// wait until there's at least one command stored in the queue
	while ((blocking == true) && (rxCmds.nQueued == 0)) {;}

	// dequeue the oldest element
	uint8_t index = mod(rxCmds.next - rxCmds.nQueued, COMMAND_QUEUE_SIZE);
	if (rxCmds.nQueued > 0)
		rxCmds.nQueued--;

	return rxCmds.cmd[index];
}

void esp_sendCommand(const union wifiCommand cmd)
{
	// block until the buffer can hold new data
	while (txCmds.nQueued == COMMAND_QUEUE_SIZE) {;}
	
	// stop interrupts when modifying the data structures
	USART_DreInterruptLevel_Set(&USARTF0, USART_DREINTLVL_OFF_gc);
	
	txCmds.cmd[txCmds.next] = cmd;
	txCmds.next = (txCmds.next + 1) % COMMAND_QUEUE_SIZE;
	txCmds.nQueued++;

	// ready to send the data out
	USART_DreInterruptLevel_Set(&ESP_USART, USART_DREINTLVL_MED_gc);
}
