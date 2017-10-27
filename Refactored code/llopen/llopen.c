
#include "llopen.h"

int llopen(int fd){
	state = START;

	unsigned char set;
	char buf[5];


	while(state!= STOP){

		read(fd, &set, 1);
		maquinaEstados(set,buf,CSET);
		//printf("SET: %x  BUF: %x",set,buf[state]);

	}
	if(buf[1]^buf[2] == A^CSET) {
		sendUA(&fd);
		return 1;
	}
	else
		return 0;
}

