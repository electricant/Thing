/**
 *
 *
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#include "include/board.h"
#include "include/utils.h"
#include "include/usart_driver.h"
#include "include/serio_driver.h"

struct SerioData {
	char data[BUFFER_SIZE];
	uint8_t next;
	uint8_t used;
};
static volatile struct SerioData rxBuf, txBuf;

void serio_init()
{
	PORTC.DIRSET = PIN3_bm; // PIN3 (TXD0) output
	PORTC.DIRCLR = PIN2_bm; // PIN2 (RXD0) input

	USART_Format_Set(&SERIO_USART, USART_CHSIZE_8BIT_gc,
		USART_PMODE_DISABLED_gc, false);
	USART_Baudrate_Set(&SERIO_USART, 11, -7); // 115200 baud (see the XMEGA manual)
	USART_Rx_Enable(&SERIO_USART);
	USART_Tx_Enable(&SERIO_USART);
	USART_RxdInterruptLevel_Set(&SERIO_USART, USART_RXCINTLVL_LO_gc);

	rxBuf.next = 0;
	rxBuf.used = 0;
	txBuf.next = 0;
	txBuf.used = 0;
}

// receive data and put it in the buffer
ISR(SERIO_USART_RXC_vect)
{
	serio_putChar(SERIO_USART.DATA);
}

// transmit data until the buffer is empty
ISR(SERIO_USART_DRE_vect)
{
	if (txBuf.used == 0) {
		// there's no need to be interrupted if nothing has to be sent
		USART_DreInterruptLevel_Set(&SERIO_USART, USART_DREINTLVL_OFF_gc);
	} else {
		uint8_t index = mod(txBuf.next - txBuf.used, BUFFER_SIZE);
		txBuf.used--;
		SERIO_USART.DATA = txBuf.data[index];
	}
}

void serio_putChar(char c)
{
	while (txBuf.used == BUFFER_SIZE) {;} // block until the buffer is empty

	// stop interrupts when modifying the data structures
	USART_DreInterruptLevel_Set(&SERIO_USART, USART_DREINTLVL_OFF_gc);

	txBuf.data[txBuf.next] = c;
	txBuf.next = (txBuf.next + 1) % BUFFER_SIZE;
	txBuf.used++;

	// (re)enable interrupt in order to send data
	USART_DreInterruptLevel_Set(&SERIO_USART, USART_DREINTLVL_LO_gc);
}

void serio_putString(char* string)
{
	while (*string != 0)
	{
		serio_putChar(*string);
		++string;
	}
}
