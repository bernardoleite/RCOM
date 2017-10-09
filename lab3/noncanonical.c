/*Non-Canonical Input Processing*/

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

unsigned char ua[5];



static int set_index;


void initialize_ua(){	
	ua[0] = FLAG;
	ua[1] = A;
	ua[2] = CUA;
	ua[3] = ua[1]^ua[2];
	ua[4] = FLAG;
} 

int main(int argc, char** argv)
{

	initialize_ua();

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

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



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

	enum State {START, FLAG_RCV, A_RCV, C_RCV, BCC_RCV, STOP};

	enum State  state = START;
	int count = 1;
	unsigned char set;
	while(1) {
	while(state!= STOP){
		read(fd, &set, 1);
		switch(state) {
			case START:
				if(set == FLAG)	
					state = FLAG_RCV;	
				break;
			case FLAG_RCV:
				if(set == A)
					state = A_RCV;
				else if (set == FLAG)
					state = FLAG_RCV;
				else
					state = START;
				break;
			case A_RCV:
				if(set == CSET)
					state = C_RCV;
				else if(set == FLAG)
			  		state = FLAG_RCV;
				else
					state = START;
				break;
			case C_RCV:
				if (set == A^CSET)
					state = BCC_RCV;
				else if (set == FLAG)
					state = FLAG_RCV;
				else
					state = START;
				break;
			case BCC_RCV:

				if (set == FLAG)
					state = STOP;
				else
					state = START;
				break;
			case STOP:
				break;

		}
	}
		printf("SET RECEIVED\n");
		if(count == 4) {
			write(fd, ua, 5);
			printf("write\n");	
			count = 0;		
		}
		count++;
		state = START;	
	}

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