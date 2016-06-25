/**
 * 
 * 
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#include "include/board.h"
#include "include/usart_driver.h"

#include "include/esp_driver.h"

// Device status
uint8_t status = 0;
// the last command received
uint16_t command;
// Received data buffer
char recBuf[RECEIVE_BUFFER_SIZE];
uint8_t bufIndex = 0;
uint8_t newLineCount = 0;

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
	char in = USART_GetChar(&USARTF0);

	if (in <= 13)
		++newLineCount;
	else
		newLineCount = 0;

	switch (newLineCount)
	{
		case 0:
		case 1: // save character in buffer
			recBuf[bufIndex] = in;
			bufIndex = (bufIndex + 1) % RECEIVE_BUFFER_SIZE;
			break;

		case 2: // parse command
			if (recBuf[2] == 'C') 
			{ // new connection on channel 0: '0,CONNECT', send greetings
            	transmitStr("AT+CIPSEND=0,14\r\n");
            	_delay_ms(1);
            	transmitStr(CONNECT_STR);
        	} else if (recBuf[1] == 'I') { // new command: '+IPD,0,length:data'
            	status |= ESP_STATUS_NEW_COMM;
            	command = (recBuf[9] << 8) | recBuf[10];
        	}
        	PORTE.OUT = ~status;
			bufIndex = 0; // New data will be put at the beginning
			break;

		default:
			;// ignore character
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
	transmitStr("AT+CWMODE=2\r\n");
	_delay_ms(10);
	transmitStr("AT+CWSAP=\"Thing\",\"\",5,0\r\n");
	_delay_ms(10);
	transmitStr("AT+CIPMUX=1\r\n");
	_delay_ms(10);
	transmitStr("AT+CIPSTO=60\r\n"); // client activity timeout
	_delay_ms(10);
	transmitStr("AT+CIPSERVER=1\r\n"); // default port = 333
	status |= ESP_STATUS_READY;
}

uint16_t esp_getCommand(bool blocking)
{
	while (blocking && (status ^ ESP_STATUS_NEW_COMM)) {;}
	status &= (ESP_STATUS_READY);
	return command;
}

void esp_sendStr(char* string)
{
	transmitStr(string); // TODO
}
