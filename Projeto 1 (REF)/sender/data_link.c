
 


#include "data_link.h"

extern int fd;
extern int conta;
extern int flag;
extern FILE *log_;
extern FILE *imagem;
void progress_bar(long long int progress, unsigned int total);

int calculate_control_packet(unsigned char indice_trama, unsigned char end_start,unsigned char* byte,unsigned char* control);
void calculate_packet(unsigned char indice_trama, long long int *file_size_inc, unsigned char* trama_informacao, unsigned char* data);


enum State state = START;

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


void maquinaEstados(unsigned char ua, char buf[], unsigned char byteControl)
{
	switch(state) {
		case START:
			if(ua == FLAG)	{
				buf[state] = ua;
				state = FLAG_RCV;	
			}
			break;
		case FLAG_RCV:
			if(ua == A)
				{
				buf[state] = ua;
				state = A_RCV;	
			}
			else if (ua == FLAG)
				state = FLAG_RCV;
			else
				state = START;
			break;
		case A_RCV:	
			if(ua == byteControl)
			{
				buf[state] = ua;
				state = C_RCV;	
			}
			else 
				if(((byteControl == RR) && (ua == REJ))
				 || ((byteControl == (RR | 0x80)) && (ua == (REJ | 0x80))))
				{
					buf[state] = ua;
					state = C_RCV;	
				}else
					if( ua == FLAG)
				  		state = FLAG_RCV;
					else
						state = START;
			break;
		case C_RCV:
			if (ua == buf[1]^buf[2])
				{
				buf[state] = ua;
				state = BCC_RCV;	
			}
			else if ( ua == FLAG)
				state = FLAG_RCV;
			else
				state = START;
			break;
		case BCC_RCV:
			if (ua == FLAG)
				{
				buf[state] = ua;
				state = STOP;	
				}
			else
				state = START;
			break;
		case STOP:
			break;

	}

}


void sendSet(int *fd){

	unsigned char set[5];
	tcflush(*fd, TCOFLUSH);

	set[0] = FLAG;
	set[1] = A;
	set[2] = CSET;
	set[3] = set[1]^set[2];
	set[4] = FLAG;

	write (*fd, set, 5);
	int i;
	for(i = 0; i < 5; i++){
		write (*fd, &(set[i]), 1);
	}
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
}

void sendDisc(int *fd){
	unsigned char disc[5];
	tcflush(*fd, TCOFLUSH);
	
	disc[0] = FLAG;
	disc[1] = A;
	disc[2] = CDISC;
	disc[3] = disc[1]^disc[2];
	disc[4] = FLAG;

	int i;
	for(i = 0; i < 5; i++){
		write (*fd, &(disc[i]), 1);
	}
}

void sendUA(int *fd){

	unsigned char ua[5];
	tcflush(*fd, TCOFLUSH);
	
	ua[0] = FLAG;
	ua[1] = A;
	ua[2] = CUA;
	ua[3] = ua[1]^ua[2];
	ua[4] = FLAG;

		
	tcflush(*fd,TCIFLUSH);
	int i;
	for(i = 0; i < 5; i++){
		write (*fd, &(ua[i]), 1);
	}
}


void llclose(int fd){
	conta=1;
	flag = 1;
	char tmp[5];
	while(conta <= MAX_TIMEOUTS) {			

		if (flag){
			alarm(3);
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
}


unsigned char receive_feedback(unsigned char control_byte_expected){
	char tmp[5];
	state = START;
	unsigned char ua;
	flag = 0;
	while ((state !=STOP) && !flag){
		
		int res = read(fd, &ua, 1);
		if(res>0)
			maquinaEstados(ua, tmp, control_byte_expected);
		
	}
	if((state==STOP) && 
	(((unsigned char)tmp[2]) == ((unsigned char)(control_byte_expected)))){
		return 1;
	}
	return 0;
}




void send_control_packet(unsigned char end_start, unsigned char control_byte_expected, unsigned char indice_trama){
	unsigned char acknowledged;
	conta = 0;
	int i;
	do{

		unsigned char *byte = (unsigned char*)malloc(5);
		unsigned char *control = (unsigned char*)malloc(9+control_packet.L2);
		
		int size_stuffed = calculate_control_packet(indice_trama, end_start, byte, control);

		for(i = 0; i < 4; i++) {
			write(fd, &(byte[i]), 1);

		}		

		for(i = 0; i < size_stuffed; i++) {
			write(fd, &(control[i]), 1);
		}	


		write(fd, &byte[4], 1);
		write(fd, &(byte[0]), 1);


		alarm(3);
		acknowledged = receive_feedback(control_byte_expected);
		alarm(0);
		

		free(byte);
		free(control);
	}while(!acknowledged && (conta < MAX_TIMEOUTS));
	if(!acknowledged){
		exit(2);
	}
}



int llwrite(int fd) {

	unsigned char indice_trama = 0;

	send_control_packet(1, R1 | RR, indice_trama);

	indice_trama = 1;

	unsigned char trama_informacao[5] = {FLAG, A};
	trama_informacao[4] = FLAG;

	long long int file_size_inc = 0;
	unsigned char acknowledged;
	unsigned char count;
	while(control_packet.V1 > file_size_inc){
		

		unsigned char* data = (unsigned char *)malloc(6 + NUMBER_BYTES_EACH_PACKER);
		calculate_packet(indice_trama, &file_size_inc, trama_informacao, data);
		int data_length = (data_packet.L2 << 8) + data_packet.L1;

		int size_stuffed = stuffing(data_length + 5, &data);

		acknowledged = 0;
		conta = 0;
		int i;
		do{		
			
			for(i = 0; i < 4; i++){
				write(fd, &trama_informacao[i], 1);

			}

			int indx;
			for(indx = 0; indx < size_stuffed; indx++){
				write(fd, &data[indx], 1);
			}

		
			write(fd, &trama_informacao[4], 1);

				
			alarm(3);	
			acknowledged = receive_feedback((indice_trama ? R0:R1) | RR);	
			alarm(0);

		}while(!acknowledged && (conta < MAX_TIMEOUTS));

		if(!acknowledged){ 
			exit(2);
		}

		
		indice_trama=!indice_trama;
		
		progress_bar(file_size_inc, control_packet.V1);
		free(data);
		
	}
	send_control_packet(0, (indice_trama ? R0:R1) | RR, indice_trama);	
}


