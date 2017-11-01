/*Non-Canonical Input Processing*/

#include "utilities.h"
#include "data_link.h"

enum State  state = START;
unsigned char ua[5];

unsigned char disc[5];


static int set_index;

extern FILE* imagem;
void sendData(char* data);


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

void sendUA(int *fd){
	unsigned char set[5];
tcflush(*fd, TCOFLUSH);

set[0] = FLAG;
set[1] = A;
set[2] = CUA;
set[3] = set[1]^set[2];
set[4] = FLAG;
int i;
  for(i = 0; i < 5; i++){ 
     write (*fd, &(set[i]), 1);
  }

}

void sendDisc(int *fd){
	unsigned char set[5];
  tcflush(*fd, TCOFLUSH);

  set[0] = FLAG;
  set[1] = A;
  set[2] = CDISC;
  set[3] = set[1]^set[2];
  set[4] = FLAG;
int i;
  for(i = 0; i < 5; i++){ 
     write (*fd, &(set[i]), 1);
  }

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

int i;
  for(i = 0; i < 5; i++){ 
     write (*fd, &(set[i]), 1);
  }
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

  int i;
  for(i = 0; i < 5; i++){ 
     write (*fd, &(set[i]), 1);
  }

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
	}
	if(buf[1]^buf[2] == A^CSET) {
		sendUA(&fd);
		printf("Conecção estabelecida\n");
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

void llread(int fd) {
	
	int times = 0, novatrama = 1;
	unsigned char Ns = 0x00, Nr = 0x40;
	while(1) {
		unsigned char td;
		unsigned char* buf = (unsigned char*)malloc(1);
	  	int n = 0, i = 0, x = 0;
		int res;
		state = START;

		while(state != STOP) {
			res = read(fd, &td, 1);
			if(res == 0)
				continue;
			maquinaEstadosTransferencia(td, buf,&n);

			buf = (unsigned char *)realloc(buf,n +1);

		}

		if(buf[2] == CDISC) {
			return;
		}

		unsigned char* dados = (unsigned char *)malloc(1);

	 	for(i = 4; i < n - 1; i++) {
			dados[x] = buf[i];

			x++;
			dados = (unsigned char *)realloc(dados, x + 1);
			
	 	}

		if((buf[1]^buf[2]) != buf[3]) {

			printf("Error in llread Head BCC\n");
			
		}
		
		x = destuffing(&dados, x);



                
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

                
        
	unsigned char bcc = 0;

	for(i = 0; i  < x - 1; i++) {
	          
		bcc = (bcc^dados[i]);
		
	}
	if(bcc == dados[x-1] && novatrama == 1) {
		sendRR(&fd, Nr);
		sendData(dados);

	}
	else if(bcc == dados[x-1] && novatrama == 0) {

		sendRR(&fd, Nr);
		fseek(imagem, -((dados[2]*256) + dados[3]), SEEK_CUR);
		sendData(dados);
	}
	else if(bcc != dados[x-1] && novatrama == 1) {

		sendRej(&fd, Nr);
	}
	else if(bcc != dados[x-1] && novatrama == 0) {
		sendRej(&fd, Nr);

	}
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
		printf("Terminação Concluída\n");
	}
	else{
		printf("Terminação Incorreta\n");
	}
}

