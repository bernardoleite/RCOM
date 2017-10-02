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

#define NUMBER_TIMEOUTS 3
#define TIMEOUT 3


#define FLAG 0x7e
#define A 0x03
#define CSET 0x03
#define CUA 0x07 
unsigned char set[5];



//int new = write(fd, set, 5);

static int set_index;
int fd, conta = 1;
void atende()                   // atende alarme
{

	write(fd, set, 5);
	set_index=0;
	conta++;

}


int main(int argc, char** argv)
{

set[0] = FLAG;
set[1] = A;
set[2] = CSET;
set[3] = set[1]^set[2];
set[4] = FLAG;
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

    newtio.c_cc[VTIME]    = NUMBER_TIMEOUTS * TIMEOUT;   /* inter-character timer unused */
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


	
    /*for (i = 0; i < 255; i++) {
      buf[i] = 'a';
    }*/
    
    /*testing*/
    //buf[25] = '\n';
    
	/*gets(buf);
	int ncar = strnlen(buf);
    res = write(fd,buf,ncar+1);*/

	(void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao



	unsigned char ua;
	

	write(fd, set, 5);	

	enum State{START, FLAG_RCV, A_RCV, C_RCV, BCC_RCV, STOP};
	enum State state;
	state = START;
	int quit = 0;
	while(conta < 3){
			
		set_index = 0;
		alarm(3);
		while(state != STOP){
			printf("STATE: %d\n",state);
			read(fd, &ua, 1);
			printf("%.2x\n",ua);
			switch(state) {
				case START:
					if(ua == FLAG)	
						state = FLAG_RCV;	
					break;
				case FLAG_RCV:
					if(ua == A)
						state = A_RCV;
					else if (ua == FLAG)
						state = FLAG_RCV;
					else
						state = START;
					break;
				case A_RCV:
					if(ua == CUA)
						state = C_RCV;

					else if( ua == FLAG)
				  		state = FLAG_RCV;
					else
						state = START;
					break;
				case C_RCV:
					if (ua == A^CUA)
						state = BCC_RCV;
					else if ( ua == FLAG)
						state = FLAG_RCV;
					else
						state = START;
					break;
				case BCC_RCV:
					printf("ua:%x\n",ua);
					printf("flag:%x\n",FLAG);
					if (ua == FLAG) {
						printf("IF\n");					
						state = STOP;
					}
					else
						state = START;
					break;
				case STOP:
					break;

			}

			//maquina estados ua		
		}
		alarm(0);

		if(state == STOP)
			break;
		


	}
printf("Vou terminar.\n");



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
