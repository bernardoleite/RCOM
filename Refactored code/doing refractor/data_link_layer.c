#include 


void llclose(){
	conta=1;
	flag = 1;
	char tmp[5];
	while(conta <= MAX_TIMEOUTS) {			

		if (flag){
			alarm(3);
			//printf("Envia disc\n");
			sendDisc(&fd);
			flag = 0;
			tcflush(fd,TCIFLUSH);

			state = START;
			while ((state !=STOP) && !flag){
				unsigned char disc;
				int res = read(fd, &disc, 1);
				if(res>0)
					maquinaEstados(disc, tmp, CDISC);
				if(tmp[3] != tmp[1]^tmp[2])
					continue;			
			}

			if(state == STOP)
				break;
		}
	}
	alarm(0);
	if(state!=STOP){
		exit(2);
	}
	sendUA(&fd);
	printf("Disconnected.\n");
}




void llopen(int fd) {
		
	conta=1;
	flag = 1;
	unsigned char ua;

	char tmp[5];
	unsigned char leave_while = 1;
	while((conta <= MAX_TIMEOUTS) && leave_while) {			

		if (flag){
			alarm(3);
			//printf("Envia set\n");
			sendSet(&fd);
			flag = 0;
			tcflush(fd,TCIFLUSH);
			state = START;
			while ( state !=STOP && !flag){
				int res = read(fd, &ua, 1);
				if(res>0)
					maquinaEstados(ua, tmp, CUA);
				if(tmp[3] != tmp[1]^tmp[2])
					continue;			
			}
			
			if(state == STOP){
				leave_while = 0;
			}
		}
	}
	if(state!=STOP){
		exit(2);
	}
	printf("Established Connection.\n");
}


void sendData(unsigned char * trama_informacao, unsigned char* data, int size_stuffed){
	//send trama header
	int i,indx;
	for(i = 0; i < 4; i++){
		write(fd, &trama_informacao[i], 1);

	}

	for(indx = 0; indx < size_stuffed; indx++){
		write(fd, &data[indx], 1);
	}

	write(fd, &trama_informacao[4], 1);
}




int llwrite() {

	unsigned char indice_trama = 0;

	send_control_packet(1, R1 | RR, indice_trama);

	indice_trama = 1;

	unsigned char trama_informacao[5] = {FLAG, A};
	trama_informacao[4] = FLAG;

	long long int file_size_inc = 0;

	unsigned char acknowledged;
	unsigned char count;
	int size_stuffed;

	while(control_packet.V1 > file_size_inc){

		acknowledged = 0;
		conta = 0;
		do{		
			file_size_inc = calculate(&size_stuffed, trama_informacao, data, file_size_inc);

			sendData(trama_informacao, data, size_stuffed);
				
			alarm(3);		
			acknowledged = receive_feedback((indice_trama ? R0:R1) | RR);	
			alarm(0);

		}while(!acknowledged && (conta < MAX_TIMEOUTS));

		if(!acknowledged){ //X REJ, Y Timeouts, X+Y = 3
			exit(2);
		}
		
		indice_trama=!indice_trama;
		
	}

	send_control_packet(0, (indice_trama ? R0:R1) | RR, indice_trama);	
}


int stuffing(int bufSize, unsigned char** buf) {
	int newBufSize = bufSize;
	
	int i;
	for (i = 1; i < bufSize; i++)
		if ((*buf)[i] == 0x7e || (*buf)[i] == 0x7d)
			newBufSize++;

	*buf = (unsigned char*) realloc(*buf, newBufSize);

	for (i = 1; i < bufSize; i++) {
		if ((*buf)[i] == 0x7e || (*buf)[i] == 0x7d) {
			memmove(*buf + i + 1, *buf + i, bufSize - i);

			bufSize++;

			(*buf)[i] = 0x7d;
			(*buf)[i + 1] ^= 0x20;
		}
	}

	return newBufSize;
}