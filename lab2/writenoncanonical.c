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

volatile int STOP=FALSE;

int flag=1, conta=1;
int flag1=0;
int fd;

void atende()                   // atende alarme
{
	printf("alarme # %d\n", conta);
	flag=1;
	conta++;
	flag1=1;
   	close(fd);
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

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 4;   /* blocking read until 5 chars received */



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

	char set = '1';
	buf[0] = '1';
	buf[1] = '\0';
	(void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

	char ua = '0';
	while(conta < 4){
			
		write(fd, buf, strnlen(buf)+1);

		printf("Fim do sleep\n");	
   		if(flag){
    		alarm(3);                 // activa alarme de 3s
			printf("%d\n",conta);
   			flag=0;
   		}

		read(fd, &ua, 1);
		
		if(flag1==1)
			fd = open(argv[1], O_RDWR | O_NOCTTY );
		flag1=0;
		if(ua == '1'){
			alarm(0);
			printf("UA received\n");
			return 0;
		}


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
