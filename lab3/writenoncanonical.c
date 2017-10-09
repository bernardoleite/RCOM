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


enum State{START, FLAG_RCV, A_RCV, C_RCV, BCC_RCV, STOP};
enum State state = START;


int fd, conta = 1, flag = 1;

int maquinaEstados(unsigned char ua, int state, char buf[])
{
printf("STATE: %d\n",state);
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
						if(ua == CUA)
							{
							buf[state] = ua;
							state = C_RCV;	
						}

						else if( ua == FLAG)
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
				return state;

}
void atende()                   // atende alarme
{

	flag = 1;		
	conta++;
	printf("TIMEOUT\n");
}


void sendSet(int *fd){

tcflush(*fd, TCOFLUSH);

set[0] = FLAG;
set[1] = A;
set[2] = CSET;
set[3] = set[1]^set[2];
set[4] = FLAG;

write (*fd, set, 5);

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


	(void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

	unsigned char ua;

	char tmp[5];

	while(conta < 4) {			

		if (flag){
			alarm(3);
			printf("Envia set\n");
			sendSet(&fd);
			flag = 0;
			tcflush(fd,TCIFLUSH);
			state = START;
			while ( state !=STOP && !flag){
				int res = read(fd, &ua, 1);
				if(res>0)
					state = maquinaEstados(ua, state, tmp);
				if(tmp[3] != tmp[1]^tmp[2])
					continue;			
			}
			printf("STATE END: %d\n",state);
			if(state == STOP)
				break;
		}


	
		/*alarm(0);

		if(state == STOP && flag == 1)
			break;*/
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
