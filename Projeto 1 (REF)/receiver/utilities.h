#ifndef UTIL_
#define UTIL_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define NUMBER_TIMEOUTS 3;
#define TIMEOUT 3;


#define FLAG 0x7e
#define A 0x03
#define CSET 0x03
#define CUA 0x07
#define CDISC 0x0B
#define CRR0 0x05
#define CRR1 0x85
#define CREJ0 0x01
#define CREJ1 0x81

enum State {START, FLAG_RCV, A_RCV, C_RCV, BCC_RCV, STOP, DATA_RCV};

#endif
