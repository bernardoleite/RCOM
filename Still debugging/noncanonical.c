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
  if(Nr == 1)
    set[2] = CRR0;
  else
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
  if(Nr == 1)
    set[2] = CREJ0;
  else
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
		printf("STATE: %d\n",state);
		read(fd, &set, 1);
		maquinaEstados(set,buf,CSET);
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

void destuffing(char *in, char *out, int size) {

  int n = 0, i;
  out[n] = in[0];
  n++;
  for(i = 1; i < size; i++) {
    if(in[i] == 0x5e && in[i-1] == 0x7d)
      out[n - 1] = 0x7e;
    else if(in[i] == 0x5d && in[i-1] == 0x7d)
      continue;
    else {
      out[n] = in[i];
      n++;
    }
  }

}

void sendData(char* data) {
	printf("Data Control: x\n",data[0]);
	if(data[0] == 0x02 || data[0] == 0x03)
		printf("Control Packet\n");
	else if(data[0] == 0x00 || data[0] == 0x40)
		printf("Data Packet\n");
}

void llread(int fd) {

	int times = 0;
	while(1) {
		unsigned char td;
		unsigned char buf[400];
	  	int n = 0, i = 0, Ns = 1, Nr = 1, novatrama = 0, x = 0;
		int res;
		state = START;
		while(state != STOP) {
			res = read(fd, &td, 1);
			if(res == 0)
				state = STOP;
			maquinaEstadosTransferencia(td, buf,&n);
			printf("Received BUF: %x TD: %x\n",buf[n-1],td);
		}

		if(buf[n - 2] == CDISC) {
			printf("Received DISC\n");		
			return;
		}

		unsigned char dados[400];
	 	for(i = 4; i < n - 2; i++) {
			dados[x] = buf[i];
			x++;
	 	}

		if((buf[1]^buf[2]) != buf[3]) {
			printf("BUF1: %x BUF2: %x BUF3: %x\n",buf[1],buf[2],buf[3]);
			printf("Error in llread Head BCC\n");
			
		}
		

		unsigned char dadosd[400];
		destuffing(dados,dadosd, x);
		unsigned char bcc = 0x00;
			if(Ns == dadosd[0])
				novatrama = 0;
			else {
				novatrama = 1;
				if(dadosd[1] == 0) {
					Ns = 1;
					Nr = 0;
				}
				else {
					Ns = 0;
					Nr = 1;
			}
		}	
		for(i = 0; i  < x ; i++) {
			bcc = (bcc^dadosd[i]);
			printf("%x, %d\n",dadosd[i], i);
		}
		printf("\n");
		printf("1BCC: %x", bcc);
		printf("BCC: %x\n Nova Trama: %d\n",buf[x], novatrama);

		if(bcc == buf[x] && novatrama == 1) {
			printf("1\n");
			sendRR(&fd, Nr);
			sendData(dadosd);
		}
		else if(bcc == buf[x] && novatrama == 0) {
			printf("2\n");
			sendRR(&fd, Nr);
		}
		else if(bcc != buf[x] && novatrama == 1) {
			printf("3\n");
			sendRej(&fd, Nr);
		}
		else if(bcc != buf[x] && novatrama == 0) {
			printf("4\n");
			sendRR(&fd, Nr);
		}
		x = 0;
		n = 0;
		times++;
		if(times == 2)
			return;
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
		printf("Terminação Concluída\n");
	}
	else
		printf("Terminação Incorreta\n");
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

    printf("New termios structure set\n");


	if(llopen(fd))
		printf("Estabelecimento Concluído\n");

	else {
		printf("Estabelecimento Incorreto\n");
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
