
#ifndef llopen
#define llopen


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "maquinas_estado.h"

int llopen(int fd);

#endif
