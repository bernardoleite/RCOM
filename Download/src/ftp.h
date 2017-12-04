#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <termios.h>

typedef struct FTP {
	int control_socket_fd;
	int data_socket_fd;
} ftp;
int read_Sock(ftp* ftp, char* code);
int connect_Sock(char* ip, int port);
void connect_ftp(ftp* ftp, char* ip, int port);
void login_Sock(ftp* ftp, char* user, char* pass);

void write_Sock(ftp* ftp, char* str);
