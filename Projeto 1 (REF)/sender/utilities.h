
#pragma once

#ifndef UTIL_
#define UTIL_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define NUMBER_TIMEOUTS 3
#define TIMEOUT 3


#define I0 0x00
#define I1 0x40
#define FLAG 0x7e
#define A 0x03
#define CSET 0x03
#define CUA 0x07 
#define CDISC 0x0B
#define RR 0x05
#define REJ 0x01
#define R1 0x80
#define R0 0x00


#define START_PACKET_CONTROL 0x02
#define END_PACKET_CONTROL 0x03

//configuration
#define NUMBER_BYTES_EACH_PACKER 0x40 //0x80
#define MAX_TIMEOUTS 3



enum State{START, FLAG_RCV, A_RCV, C_RCV, BCC_RCV, STOP};


struct Control_packet{
	unsigned char C;
	unsigned char T1;
	unsigned char L1;
	unsigned int V1;
	unsigned char T2;
	unsigned char L2;
	char *V2;
};


struct Data_packet{
	unsigned char C;
	unsigned char N;
	unsigned char L2;
	unsigned char L1;
	char *P; 
};

#endif
