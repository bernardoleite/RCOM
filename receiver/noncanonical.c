/*Non-Canonical Input Processing*/
/*
Acabar write
Depois do read (criar imagem com o nome e escrever)
Ler imagem
Refactor
Alguma Documentaçao
*/

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

enum State  state = START;
unsigned char ua[5];

unsigned char disc[5];

FILE* imagem;
static int set_index;



void sendUA(int *fd){
	unsigned char set[5];
tcflush(*fd, TCOFLUSH);

set[0] = FLAG;
set[1] = A;
set[2] = CUA;
set[3] = set[1]^set[2];
set[4] = FLAG;

write (*fd, set, 5);

}

void sendDisc(int *fd){
	unsigned char set[5];
  tcflush(*fd, TCOFLUSH);

  set[0] = FLAG;
  set[1] = A;
  set[2] = CDISC;
  set[3] = set[1]^set[2];
  set[4] = FLAG;

  write (*fd, set, 5);

}

void sendRR(int *fd,int Nr){
	unsigned char set[5];
  tcflush(*fd, TCOFLUSH);

  set[0] = FLAG;
  set[1] = A;
  if(Nr == 0x00)
    set[2] = CRR0;
  else if(Nr == 0x40)
    set[2] = CRR1;
  set[3] = set[1]^set[2];
  set[4] = FLAG;

  write (*fd, set, 5);

}

void sendRej(int *fd, int Nr){
	unsigned char set[5];
  tcflush(*fd, TCOFLUSH);

  set[0] = FLAG;
  set[1] = A;
  if(Nr == 0x00)
    set[2] = CREJ0;
  else if(Nr == 0x40)
    set[2] = CREJ1;
  set[3] = set[1]^set[2];
  set[4] = FLAG;

  write (*fd, set, 5);

}
void maquinaEstados(unsigned char tmp, char buf[], unsigned char byteControl) {

		switch(state) {
			case START:
				if(tmp == FLAG)	{
					state = FLAG_RCV;
					buf[state] = tmp;
				}
				break;
			case FLAG_RCV:
				if(tmp == A) {
					state = A_RCV;
					buf[state] = tmp;
				}
				else if (tmp == FLAG)
					state = FLAG_RCV;
				else
					state = START;
				break;
			case A_RCV:
				if(tmp == byteControl) {
					state = C_RCV;
					buf[state] = tmp;
				}
				else if(tmp == FLAG)
			  		state = FLAG_RCV;
				else
					state = START;
				break;
			case C_RCV:

				if (tmp == buf[1]^buf[2]) {
					state = BCC_RCV;
					buf[state] = tmp;
				}
				else if (tmp == FLAG)
					state = FLAG_RCV;
				else
					state = START;
				break;
			case BCC_RCV:

				if (tmp == FLAG) {
					state = STOP;
					buf[state] = tmp;
				}
				else
					state = START;
				break;
			case STOP:
				break;

		}


}

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

void maquinaEstadosTransferencia(unsigned char td, char buf[], int* n) {

	switch(state) {
		case START:
			if(td == FLAG)	{
				state = FLAG_RCV;
				buf[(*n)++] = td;
			}
			break;
		case FLAG_RCV:
			if(td == A) {
				state = A_RCV;
				buf[(*n)++] = td;
			}
			else if (td == FLAG) {
				state = FLAG_RCV;
				*n = 1;
			}
			else {
				state = START;
				*n = 0;
			}
			break;
		case A_RCV:
			if(td == 0x00 || td == 0x40 || td == CDISC) {
				state = C_RCV;
				buf[(*n)++] = td;
			}
			else if(td == FLAG) {
		  		state = FLAG_RCV;
				*n = 1;
			}
			else {
				state = START;
				*n = 0;
			}
			break;
		case C_RCV:
			if (td == buf[1]^buf[2]) {
				state = DATA_RCV;
				buf[(*n)++] = td;
			}
			else if (td == FLAG) {
				state = FLAG_RCV;
				*n = 1;
			}
			else {
				state = START;
				*n = 0;
			}
			break;
		case STOP:
			buf[(*n)++] = td;
			break;
		case DATA_RCV:
			if(td == FLAG) {
				state = STOP;
				buf[(*n)++] = td;
			}

			else
				buf[(*n)++] = td;
			break;
	}
}

int destuffing(unsigned char **buf, int bufSize) {

	int i;
	for (i = 1; i < bufSize - 1; ++i) {
		if ((*buf)[i] == 0x7d) {
			memmove(*buf + i, *buf + i + 1, bufSize - i - 1);

			bufSize--;

			(*buf)[i] ^= 0x20;
		}
	}

	*buf = (unsigned char*) realloc(*buf, bufSize);

	return bufSize;
}

void writeData(char* data) {

	int i;
	

	if(data[0] == 0x02) {

		char* filename = (char*) malloc(data[8] + 1);
		for(i = 0; i < data[8]; i++) {
			filename[i] = data[9 + i];
                        //printf("%c",data[9+i]);
		}
		//printf("\n");

		imagem = fopen(filename,"wb");
		if(imagem == NULL) {

			exit(0);
		}		

		free(filename);

	}else if(data[0] == 0x03) {
			fclose(imagem);
					char* filename = (char*) malloc(data[8] + 1);
			for(i = 0; i < data[8]; i++) {
				filename[i] = data[9 + i];
		                    //printf("%c",data[9+i]);
			}
			//printf("\n");
		}
		else if( data[0] == 0x01) {

			char* packet = (char *) malloc((data[2]*256) + data[3]);
			for(i = 0; i < ((data[2]*256) + data[3]); i++) {
				packet[i] = data[4 + i];

			}


		fwrite(packet, 1, ((data[2]*256) + data[3]), imagem);
		free(packet);

	}else
		printf("Something wrong sending data\n");
}

void llread(int fd) {
	
	int times = 0, novatrama = 1;
	unsigned char Ns = 0x00, Nr = 0x40;
	while(1) {
		unsigned char td;
		unsigned char* buf = (unsigned char*)malloc(1);
	  	int n = 0, i = 0, x = 0;
		int res;
		state = START;
		//printf("\n");
		//printf("\n");
		while(state != STOP) {
			res = read(fd, &td, 1);
			if(res == 0)
				continue;
			maquinaEstadosTransferencia(td, buf,&n);
			//printf("TD: %x  BUF: %x N:%d\n",td,buf[n-1],n);
			buf = (unsigned char *)realloc(buf,n +1);

		}
		//printf("\n\n");
		if(buf[2] == CDISC) {
			return;
		}

		unsigned char* dados = (unsigned char *)malloc(1);

		//printf("\nFLAG: %x A: %x C: %x BCC1: %x \n",buf[0],buf[1],buf[2],buf[3]);
	 	for(i = 4; i < n - 1; i++) {
			dados[x] = buf[i];

			x++;
			dados = (unsigned char *)realloc(dados, x + 1);
			
	 	}

		if((buf[1]^buf[2]) != buf[3]) {

			//printf("Error in llread Head BCC\n");
			
		}
		

		//printf("X: %d\n",x);
		
		x = destuffing(&dados, x);


                //printf("\nBCC2: %x  FLAG: %x\n",dados[x-1],buf[n-1]);
		//printf("\n");
		
                
			if(Ns != buf[2])
				novatrama = 0;
			else {
				novatrama = 1;
				if(buf[2] == 0x00) {
					Ns = 0x40;
					Nr = 0x40;
				}
				else if(buf[2] == 0x40){
					Ns = 0x00;
					Nr = 0x00;
			}
		}	

                
                
		//printf("NOVATRAMA: %d\n",novatrama);
		unsigned char bcc = 0;

		for(i = 0; i  < x - 1; i++) {
		          
			//printf(" >>%x<< ",dados[i]);
			bcc = (bcc^dados[i]);
			//printf("I= %d DADOS::: %x \n",i,dados[i]);
			
		}
		//printf("\n");
		//printf("%x = %x\n",bcc,dados[x-1]);
		if(bcc == dados[x-1] && novatrama == 1) {
			sendRR(&fd, Nr);
			writeData(dados);
		}
		else if(bcc == dados[x-1] && novatrama == 0) {
		//printf("\n1!!!!!!!!BCCCalculado: %x   123213442334\n",bcc);
		//printf("\n1!!!!!!!!BCC: %x   123213442334\n",buf[n-2]);
			sendRR(&fd, Nr);
			fseek(imagem,-((dados[2]*256) + dados[3]),SEEK_CUR);
			writeData(dados);
		}
		else if(bcc != dados[x-1] && novatrama == 1) {
		//printf("\n2!!!!!!!!BCCCalculado: %x\n",bcc);
		//printf("\n2!!!!!!!!BCC: %x\n",buf[n-2]);

			sendRej(&fd, Nr);
			printf("bcc!");
		}
		else if(bcc != dados[x-1] && novatrama == 0) {
		//printf("\n3!!!!!!!!BCCCalculado: %x\n",bcc);
		//printf("\n3!!!!!!!!BCC: %x\n",buf[n-2]);
			sendRej(&fd, Nr);
			printf("bcc!");

		}
		else
			printf("error");
		x = 0;
		n = 0;
		free(buf);
		free(dados);
		

	}
}




void llclose(int fd) {
	state = START;
	sendDisc(&fd);
	unsigned char rec;
	unsigned char buf[5];
	while(state != STOP){
		read(fd, &rec, 1);
		maquinaEstados(rec, buf,CUA);
	}
	if(buf[1]^buf[2] == A^CUA) {
		//printf("Terminação Concluída\n");
	}
	else{
		//printf("Terminação Incorreta\n");
	}
}



int main(int argc, char** argv)
{

    int c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

	int fd;
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


	if(llopen(fd)){
		//printf("Estabelecimento Concluído\n");
        }
	else {
		//printf("Estabelecimento Incorreto\n");
		close(fd);
		exit(0);
	}
	llread(fd);

	llclose(fd);
	
  /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
  */




    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}
