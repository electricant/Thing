/**
 * Tester program for 'thing'.
 *
 * This program sends commands through the wifi link in order to move the
 * servos. See its command line options for usage.
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
#include <time.h>

#include "include/board.h" // use the same header as the firmware

#define BOARD_IP   "192.168.4.1"
#define BOARD_PORT 333

const char* USAGE_STR = "Usage: %s [options] [value]\n\n"
                        "Options:\n"
                        "   -a\tSet the angle value\n"
                        "   -c\tSet maximum current\n"
                        "   -h\tDisplay this help text\n"
                        "   -m\tChange operating mode (A, H or F)\n"
                        "   -n\tSet servo number\n"
                        "   -s\tSet rotation speed\n";

void parseCmdLine(int argc, char *argv[],int* m_ptr, int*n_ptr, int* a_ptr,
			int* s_ptr, int* c_ptr)
{
	int opt;
	while ((opt = getopt(argc, argv, "ha:c:m:n:s:")) != -1) {
		const char* errstr;
		switch (opt) 
		{
			case 'a':
				*a_ptr = strtonum(optarg, 0, 180, &errstr);
				if (errstr != NULL) {
					fprintf(stderr, "Could not parse the angle. Reason: %s\n",
						  errstr);
					exit(EXIT_FAILURE);
				}
				break;
			case 'c':
				*c_ptr = strtonum(optarg, 0, 255, &errstr);
				if (errstr != NULL) { 
					fprintf(stderr, "Could not parse the current. Reason: %s\n",
						  errstr);
					exit(EXIT_FAILURE);
				}
				break;
			case 'h':
				printf(USAGE_STR, argv[0]);
				exit(0);
				break;
			case 'm':
				if (*optarg == 'F')
					*m_ptr = WIFI_MODE_FOLLOW;
				else if (*optarg == 'A')
					*m_ptr = WIFI_MODE_ANGLE;
				else if (*optarg == 'H')
					*m_ptr = WIFI_MODE_HOLD;
				else { 
					fprintf(stderr, "Invalid mode: %s. Expected F, A or H\n",
						optarg);
					exit(EXIT_FAILURE);
				}
				break;
			case 'n':
				*n_ptr = strtonum(optarg, 0, 5, &errstr);
				if (errstr != NULL) { 
					fprintf(stderr, "Could not parse the servo number."
					" Reason: %s\n", errstr);
					exit(EXIT_FAILURE);
				}
				break;
			case 's':
				*s_ptr = strtonum(optarg, 0, 255, &errstr);
				if (errstr != NULL) { 
					fprintf(stderr, "Could not parse the speed. Reason: %s\n",
						  errstr);
					exit(EXIT_FAILURE);
				}
				break;
			default:
				fprintf(stderr, USAGE_STR, argv[0]);
				exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char *argv[])
{
	int mode = -1;
	int servo_number = -1;
	int angle = -1;
	int speed = -1;
	int current = -1;

	if (argc == 1) {
		printf("Nothing to do.\n");
		fprintf(stderr, USAGE_STR, argv[0]);
		return 0;
	}

	parseCmdLine(argc, argv, &mode, &servo_number, &angle, &speed, &current);

	// check command line options
	if (((angle != -1) || (speed != -1) || (current != -1))
			&& (servo_number == -1)) {
		fprintf(stderr, "Please provide a servo to move\n");
		exit(EXIT_FAILURE);
	}

	// We can open the connection now
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
	
	// wait some time after succesful connection
	struct timespec delay, rem;
	delay.tv_sec = 0;
	delay.tv_nsec = 20*1000*1000; // 15 ms
	nanosleep(&delay, &rem);

	union wifiCommand_le cmd;

	if (mode != -1) { // change mode first
		cmd.field.command = WIFI_SET_MODE;
		cmd.field.data = mode;
		send(socket_desc , &cmd, sizeof(cmd), 0);
		nanosleep(&delay, &rem);
	}

	if (angle != -1) {
		cmd.field.command = WIFI_SET_ANGLE;
		cmd.field.servo = servo_number;
		cmd.field.data = angle;
		send(socket_desc , &cmd, sizeof(cmd), 0);
		nanosleep(&delay, &rem);
	}

    if (current != -1) {
        cmd.field.command = WIFI_SET_CURRENT;
        cmd.field.servo = servo_number;
        cmd.field.data = current;
        send(socket_desc , &cmd, sizeof(cmd), 0);
    	nanosleep(&delay, &rem);
    }

    if (speed != -1) {
        cmd.field.command = WIFI_SET_SPEED;
        cmd.field.servo = servo_number;
        cmd.field.data = speed;
        send(socket_desc , &cmd, sizeof(cmd), 0);
        nanosleep(&delay, &rem);
    }

	// wait for the remote end to close the socket
	int len = 1;
	while (len > 0) {
		char buffer;
		len = read(socket_desc, &buffer, len);
	}
	printf("Connection closed\n");
	return 0;
}
