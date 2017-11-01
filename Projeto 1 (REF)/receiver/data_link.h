#ifndef REC_
#define REC_

void sendUA(int *fd);
void sendDisc(int *fd);
void sendRR(int *fd,int Nr);
void sendRej(int *fd, int Nr);

int llopen(int fd);
void llread(int fd);
void llclose(int fd);
int destuffing(unsigned char **buf, int bufSize);

#endif
