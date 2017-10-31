#ifndef RECEIVER_H
#define RECEIVER_H

/*Functions Responsible of Application Layer*/
void maquinaEstados(unsigned char tmp, char buf[], unsigned char byteControl);
void maquinaEstadosTransferencia(unsigned char td, char buf[], int* n);
void treatData(char* data);
/*Functions Responsible of Application Layer*/

/*Functions Responsible of Data Link Layer*/
void llread(int fd);
void llclose(int fd);
int llopen(int fd);
void sendUA(int *fd);
void sendDisc(int *fd);
void sendRR(int *fd,int Nr);
void sendRej(int *fd, int Nr);
int destuffing(unsigned char **buf, int bufSize);
/*Functions Responsible of Data Link Layer*/



#endif