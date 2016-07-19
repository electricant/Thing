/**
 * Tester program for 'thing'.
 *
 * This program sends commands through the wifi link and listens to the answer.
 * The goal is to monitor current, speed and angle of each servo at a fixed
 * rate.
 *
 * NOTE: due to the fact that answers are always sent to channel 0 of the
 * ESP8266, this should be the first test program to be launched.
 *
 * Copyright (C) 2015 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <bsd/stdlib.h> // requires libbsd-dev
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "include/board.h" // use the same header as the firmware

#define BOARD_IP   "192.168.4.1"
#define BOARD_PORT 333

const char* USAGE_STR = "Usage: %s [timeout]\n\n"
                        "Options;\n"
                        "    timeout\tTimeout in seconds before asking again "
                        "for data. The timeout has to be between 1 (default) "
                        "and 60\n";

int main(int argc, char *argv[])
{
	int timeout = 1; // default timeout

	// parse rate (if any)
	if (argc == 2) {
		const char* estr;
		timeout = strtonum(argv[1], 2, 1000, &estr);

		if (estr != NULL) {
			fprintf(stderr, "Could not parse the rate. Reason: %s\n\n", estr);
			printf(USAGE_STR, argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// create the socket
	int socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		fprintf(stderr, "Could not create socket.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in board;
	board.sin_addr.s_addr = inet_addr(BOARD_IP);
	board.sin_family = AF_INET;
	board.sin_port = htons(BOARD_PORT);

	if (connect(socket_desc, (struct sockaddr *)&board , sizeof(board)) < 0)
	{
		fprintf(stderr, "Error connecting to target board\n");
		exit(EXIT_FAILURE);
	}

	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
			sizeof(struct timeval));

	// loop
	union wifiCommand_le cmd; // Sending is little endian whereas receiving is
	union wifiCommand answer; // not. This is rather strange...
	cmd.field.servo = 0;
	while (1)
	{
		printf("Servo %u:\n", cmd.field.servo);

		cmd.field.command = WIFI_GET_ANGLE;
		int retval = -1;
		while (retval < 0) {
			send(socket_desc , &cmd, sizeof(cmd), 0);
			retval = read(socket_desc, &answer, sizeof(answer));
		}
		printf("\tAngle: %u\n", answer.field.data);

		cmd.field.command = WIFI_GET_CURRENT;
		retval = -1;
		while (retval < 0) {
			send(socket_desc , &cmd, sizeof(cmd), 0);
			retval = read(socket_desc, &answer, sizeof(answer));
		}
		printf("\tCurrent: %u\n", answer.field.data);

		cmd.field.command = WIFI_GET_SPEED;
		retval = -1;
		while (retval < 0) {
			send(socket_desc , &cmd, sizeof(cmd), 0);
			retval = read(socket_desc, &answer, sizeof(answer));
		}
		printf("\tSpeed: %u\n", answer.field.data);

		cmd.field.servo = (cmd.field.servo + 1) % 5;
		sleep(1);
	}

	return 0;
}
