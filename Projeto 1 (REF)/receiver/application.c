
#include "utilities.h"
#include "data_link.h"

FILE* imagem;


void sendData(char* data) {

	int i;
	

	if(data[0] == 0x02) {

		char* filename = (char*) malloc(data[8] + 1);
		for(i = 0; i < data[8]; i++) {
			filename[i] = data[9 + i];
		}

		imagem = fopen(filename,"wb");
		if(imagem == NULL) {

			exit(0);
		}		

		free(filename);

	}
	if(data[0] == 0x03) {
		fclose(imagem);
				char* filename = (char*) malloc(data[8] + 1);
		for(i = 0; i < data[8]; i++) {
			filename[i] = data[9 + i];
		}
	}
	else if(data[0] == 0x00 || data[0] == 0x01) {

		char* packet = (char *) malloc((data[2]*256) + data[3]);
		for(i = 0; i < ((data[2]*256) + data[3]); i++) {
			packet[i] = data[4 + i];

		}

		fwrite(packet, 1, ((data[2]*256) + data[3]), imagem);
		free(packet);

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

	int fd;
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) {
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;


    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 10;  
    newtio.c_cc[VMIN]     = 0;   


    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }


    if(!llopen(fd)){
	close(fd);
	exit(0);
    }
    llread(fd);
    llclose(fd);
	



    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}
