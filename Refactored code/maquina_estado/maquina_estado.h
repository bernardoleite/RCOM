#ifndef maquina_estado
#define maquina_estado

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

enum State {START, FLAG_RCV, A_RCV, C_RCV, BCC_RCV, STOP, DATA_RCV};

enum State  state = START;

void maquinaEstados(unsigned char tmp, char buf[], unsigned char byteControl);

#endif
