#include "ftp.h"

int read_Sock(ftp* ftp, char* str) {
	FILE* fp = fdopen(ftp->control_socket_fd, "r");
	size_t bufsize = 32, c;
	char* msg = (char *)malloc(bufsize * sizeof(char));

	do {

		msg = fgets(msg, 5, fp);
		printf("%s\n",msg);
	} while (!('1' <= msg[0] && msg[0] <= '5') || msg[3] != ' ');

	free(msg);
	return 0;
}

int connect_Sock(char* ip, int port) {

	int sockfd;
	struct sockaddr_in server_addr;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */
    
	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(0);
    	}
	/*connect to the server*/
    	if(connect(sockfd, 
	           (struct sockaddr *)&server_addr, 
		   sizeof(server_addr)) < 0){
        	perror("connect()");
		exit(0);
	}
	return sockfd;
}

void connect_ftp(ftp* ftp, char* ip, int port) {
	int sockfd;

	if((sockfd = connect_Sock(ip, port)) < 0) {
		fprintf(stderr,"usage: Error connecting socket\n");
		exit(1);
	}
	ftp->control_socket_fd = sockfd;
	ftp->data_socket_fd = 0;
	char rd[1024];
	if(read_Sock(ftp,rd)) {
		fprintf(stderr,"usage: Error connecting socket\n");
		exit(1);
		
	} 
}

void write_Sock(ftp* ftp, char* str) {
	int nBytes = 0;
	if ((nBytes = write(ftp->control_socket_fd, str, strlen(str))) <= 0) {
		fprintf(stderr,"usage: Number of Bytes Written wrong %d\n",nBytes);
		exit(1);	
	}
	printf("Bytes send: %d\nInfo: %s\n", nBytes, str);
}
