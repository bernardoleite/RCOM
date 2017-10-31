#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "sender.h"

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
#define NUMBER_BYTES_EACH_PACKER 0x0a //0x40
#define MAX_TIMEOUTS 3



enum State{START, FLAG_RCV, A_RCV, C_RCV, BCC_RCV, STOP};
enum State state = START;


int fd, conta, flag;
FILE *imagem;


//////////ELIMNAR
FILE *log_;
//////////ELIMNAR

struct Control_packet{
	unsigned char C;
	unsigned char T1;
	unsigned char L1;
	unsigned int V1;
	unsigned char T2;
	unsigned char L2;
	char *V2;
}control_packet;


struct Data_packet{
	unsigned char C;
	unsigned char N;
	unsigned char L2;
	unsigned char L1;
	char *P; 
}data_packet;


//data_link_layer
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

//application
void read_data_into_packet(){
	static char sequence_number = 0; //??????? 0 1 0 1 0 1 2 3 4
	data_packet.N = ++sequence_number;
	if(sequence_number == 255)	
		sequence_number = 0;

	int read_n_bytes = fread(data_packet.P, 1, NUMBER_BYTES_EACH_PACKER, imagem);
	
	fwrite(data_packet.P,1,10,log_);


	//printf("Size of file:%d\n", read_n_bytes);
	if(read_n_bytes != NUMBER_BYTES_EACH_PACKER){
		
		data_packet.L2 = read_n_bytes >> 8;
		data_packet.L1 = (unsigned char)read_n_bytes;
	}
	
	int i;
	for(i=0; i < read_n_bytes; i++){
		//printf("%x ",data_packet.P[i]);
	}

}

void maquinaEstados(unsigned char ua, char buf[], unsigned char byteControl)
{
	//printf("STATE:%d\n",state);
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
			//printf("UA:%x Bytecontrolo:%x\n",ua,byteControl);	
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
void atende()                   
{

	flag = 1;		
	conta++;
	printf("TIMEOUT\n");
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
}

//data_link_layer
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



void sendDisc(int *fd){
	//printf("HERE!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	unsigned char disc[5];
	tcflush(*fd, TCOFLUSH);
	
	disc[0] = FLAG;
	disc[1] = A;
	disc[2] = CDISC;
	disc[3] = disc[1]^disc[2];
	disc[4] = FLAG;

	write (*fd, disc, 5);
}

void sendUA(int *fd){

	unsigned char ua[5];
	tcflush(*fd, TCOFLUSH);
	
	ua[0] = FLAG;
	ua[1] = A;
	ua[2] = CUA;
	ua[3] = ua[1]^ua[2];
	ua[4] = FLAG;

	write (*fd, ua, 5);
}

//data_link_layer
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



unsigned char receive_feedback(unsigned char control_byte_expected){
	char tmp[5];
	state = START;
	unsigned char ua;
	flag = 0;
	while ((state !=STOP) && !flag){
		
		int res = read(fd, &ua, 1);
		if(res>0)
			maquinaEstados(ua, tmp, control_byte_expected);
		//printf("RECEIVED: %x\n", ua);
		
	}
	if((state==STOP) && 
	(((unsigned char)tmp[2]) == ((unsigned char)(control_byte_expected)))){
		return 1;
	}
	
	return 0;
}

//data_link_layer
void WriteControlPacket(int size, unsigned char bcc2, unsigned char * control, unsigned char * byte){

	int size_stuffed = stuffing(size, &control);

	int i ;

	for(i = 0; i < 4; i++) {
		write(fd, &(byte[i]), 1);

	}		

	for(i = 0; i < size_stuffed; i++) {
		write(fd, &(control[i]), 1);

	}	


	write(fd, &bcc2, 1);
	write(fd, &(byte[0]), 1);
}


//application
void send_control_packet(unsigned char end_start, unsigned char control_byte_expected, unsigned char indice_trama){
	unsigned char acknowledged;
	conta = 0;
	int i;

	//printf("sending control packet\n");

	unsigned char *byte = (unsigned char*)malloc(4);
	unsigned char *control = (unsigned char*)malloc(9+control_packet.L2);
	
	byte[0] = FLAG;
	byte[1] = A;
	byte[2] = (indice_trama ? I1:I0);
	byte[3] = A^byte[2];


	control[0] = (end_start ? START_PACKET_CONTROL : END_PACKET_CONTROL);
	control[1]	= control_packet.T1;
	control[2]	= control_packet.L1;
			
	control[3]	= ((char*)&control_packet.V1)[3];
	control[4]	= ((char*)&control_packet.V1)[2];
	control[5]	= ((char*)&control_packet.V1)[1];
	control[6] = ((char*)&control_packet.V1)[0];

	control[7]	= control_packet.T2;
	control[8]	= control_packet.L2;

	unsigned char bcc2 = 0;

	do{

		
		for(i = 0; i < control_packet.L2; i++){
			control[9+i] = control_packet.V2[i];

		}
		int size = 9+i;
		for(i = 0; i < size; i++) {
			bcc2 ^= control[i];

		}

		//data_link_layer
		WriteControlPacket(size, bcc2, control, byte);


		alarm(3);
		acknowledged = receive_feedback(control_byte_expected);
		alarm(0);

	}while(!acknowledged && (conta < MAX_TIMEOUTS));

	free(byte);
	free(control);
	if(!acknowledged){
		exit(2);
	}
}

//data_link_layer
void writeData(int data_length, unsigned char * trama_informacao, unsigned char* data){
	

	int size_stuffed = stuffing(data_length + 5, &data);

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

//application
int llwrite(int fd) {

	unsigned char indice_trama = 0;

	//application
	send_control_packet(1, R1 | RR, indice_trama);

	indice_trama = 1;

	unsigned char trama_informacao[5] = {FLAG, A};
	trama_informacao[4] = FLAG;

	long long int file_size_inc = 0;

	unsigned char acknowledged;
	unsigned char count;
	unsigned char * data;

	while(control_packet.V1 > file_size_inc){

		acknowledged = 0;
		conta = 0;
		do{		
			if(indice_trama){
			trama_informacao[2] = I1;
			trama_informacao[3] = A^I1;
		}else{
			trama_informacao[2] = I0;
			trama_informacao[3] = A^I0;
		}

		read_data_into_packet();

		int i;
		int data_length = (data_packet.L2 << 8) + data_packet.L1;

		file_size_inc+=data_length;

		data = (unsigned char *)malloc(6 + data_length);
		data[0] = data_packet.C;
		data[1] = data_packet.N;
		data[2] = data_packet.L2;
		data[3] = data_packet.L1;

		unsigned char bcc2 = 0;
		for(i=0; i < 4; i++){
			bcc2 ^= data[i];			
		}

		for(i = 0; i < data_length; i++) {
			data[4 + i] = data_packet.P[i];
			bcc2 ^= data[4 + i];
		}	
		int x;
		data[i+4] = bcc2;

		//data_link_layer
		writeData(data_length, trama_informacao, data);
				
		alarm(3);		
		acknowledged = receive_feedback((indice_trama ? R0:R1) | RR);	
		alarm(0);

		}while(!acknowledged && (conta < MAX_TIMEOUTS));

		if(!acknowledged){ //X REJ, Y Timeouts, X+Y = 3
			exit(2);
		}
		
		indice_trama=!indice_trama;
		
	}
	
	//application
	send_control_packet(0, (indice_trama ? R0:R1) | RR, indice_trama);	
}

//application
void setup_data_packet(){

	data_packet.N = 0;
	data_packet.C = 0x01;
	data_packet.L2 = (unsigned char)(NUMBER_BYTES_EACH_PACKER >> 8);
	data_packet.L1 = (unsigned char)NUMBER_BYTES_EACH_PACKER;

	data_packet.P = (unsigned char*)malloc((short)NUMBER_BYTES_EACH_PACKER);
	
}

//application
void setup_control_packet(char *argv){

	long long int file_size;
	fseek(imagem, 0, SEEK_END);
	file_size=ftell(imagem);
	rewind(imagem);

	control_packet.T1 = 0x00;
	control_packet.T2 = 0x01;
	control_packet.C = 0x02;
	control_packet.L1 = sizeof(long long int);
	control_packet.V1 = file_size;

	control_packet.L2 = strlen(argv);
	control_packet.V2 = (char *)malloc(control_packet.L2);
	int i;
	for(i = 0; i < control_packet.L2; i++){
		control_packet.V2[i] = argv[i];
	}
}



int main(int argc, char** argv){

    int c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 3) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 10;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    //printf("New termios structure set\n");


    (void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

    imagem = fopen(argv[2], "rb");

	log_ = fopen("log.txt", "w");

	
    if(!imagem){
		printf("Image could not be found!\n");
        exit(3);
    }

/*Application*/
    setup_control_packet(argv[2]);
    setup_data_packet();


    llopen(fd);
 
    llwrite(fd);

    llclose(fd);
/*Application*/


    free(data_packet.P);
    free(control_packet.V2);

	fclose(log_);

  //  printf("Vou terminar.\n");



  /* 
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 
    o indicado no guião 
  */



   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }


    fclose(imagem);

    close(fd);
    return 0;
}