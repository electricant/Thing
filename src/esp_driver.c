/**
 * This driver is responsible for controlling the ESP8266 module and for command
 * transmission and reception.
 *
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#include "include/esp_driver.h"
#include "include/serio_driver.h" // for debug purposes

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

/**
 * Command transmission is done in two steps.
 * In the first one CMD_SEND is sent through the serial port. This enables the
 * transmission.
 * In the second step the actual data is sent. The command is two byte wide so
 * we have to keep count of how many bytes have been sent.
 * The following variables make such functionality possible.
 */
const static char* CMD_SEND = "AT+CIPSEND=0,2\r\n";
static volatile uint8_t initCount = 0;   // index within the string above
static volatile uint8_t cmdCount = 0;    // index within the command
static volatile bool    canSend = false; // is the device ready to receive our data?

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
	USART_Format_Set(&ESP_USART, USART_CHSIZE_8BIT_gc, USART_PMODE_DISABLED_gc,
		false);
	USART_Baudrate_Set(&ESP_USART, 11, -7); // 115200 baud (see the XMEGA manual)
	USART_Rx_Enable(&ESP_USART);
	USART_Tx_Enable(&ESP_USART);

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
	transmitStr("AT+CIPSERVER=1\r\n"); // default port = 333
	//_delay_ms(10);
	//transmitStr("AT+CIPSTO=60\r\n"); // client activity timeout

	// initialize an empty command Queue
	txCmds.next = 0;
	txCmds.nQueued = 0;
	rxCmds.next = 0;
	rxCmds.nQueued = 0;

	// Ready to enable interrupts
	USART_RxdInterruptLevel_Set(&ESP_USART, USART_RXCINTLVL_HI_gc);
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
			} else if (in == '>') {
				canSend = true;
				serio_putString("send\r\n");
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
		// nothing to do. Stop
		USART_DreInterruptLevel_Set(&ESP_USART, USART_DREINTLVL_OFF_gc);
	} else {
		char ch = *(CMD_SEND + initCount);

		if (ch != 0) {
			ESP_USART.DATA = ch;
			++initCount;
		} else if (canSend == true) { // string ended and device ready
			uint8_t index = mod(txCmds.next - txCmds.nQueued,
						COMMAND_QUEUE_SIZE);
			char* cmd_ptr = (char*) &txCmds.cmd[index];
			ch = *(cmd_ptr + cmdCount);
			ESP_USART.DATA = ch;

			cmdCount = (cmdCount + 1) % 2;
			if (cmdCount == 0) {
				--txCmds.nQueued;
				canSend = false;
				initCount = 0;
			}
		}
	}
}

void transmitStr(char* data)
{
	while (*data != 0)
	{
		while(!USART_IsTXDataRegisterEmpty(&ESP_USART)) {;}
		USART_PutChar(&ESP_USART, *data);
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
	// stop interrupts when modifying the data structures
	USART_DreInterruptLevel_Set(&ESP_USART, USART_DREINTLVL_OFF_gc);

	txCmds.cmd[txCmds.next] = cmd;
	txCmds.next = (txCmds.next + 1) % COMMAND_QUEUE_SIZE;

	if (txCmds.nQueued < COMMAND_QUEUE_SIZE) // discard old data
		txCmds.nQueued++;

	// ready to send the data out
	USART_DreInterruptLevel_Set(&ESP_USART, USART_DREINTLVL_LO_gc);
}
