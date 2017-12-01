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



int main(int argc, char** argv) {

	if (argc != 2) {  
		fprintf(stderr,"usage: wrong number of arguments\n");
		exit(1);
	}
	
	char* url_init = malloc(6 * sizeof(char));
	memcpy(url_init, argv[1], 6);
	
	if(strcmp(url_init, "ftp://\0") != 0) {
		fprintf(stderr,"usage: start the url with ftp://\n");
		exit(1);
	}
	
	char** url_aux = malloc(3 * sizeof(char *));
	int size = strlen(argv[1]) - 6;
	url_aux[0] = malloc(size);
	memcpy(url_aux[0], argv[1] + 6, size);
	url_aux[0][size] = '\0';

	char* user = malloc(strlen(url_aux[0]));
	char* pass = malloc(strlen(url_aux[0]));
	user[0] = '\0';
	pass[0] = '\0';
	if(url_aux[0][0] == '[') {

	
		memcpy(user, url_aux[0] + 1, strlen(url_aux[0]));
		strtok(user, ":");

		if(strlen(user) == strlen(url_aux[0])-1) {
			fprintf(stderr,"usage: Declare an user\n");
			exit(1);
		}
		printf("USER: %s\n",user);

		memcpy(pass, url_aux[0] + 2 + strlen(user), strlen(url_aux[0]));
		strtok(pass, "@");

		if(strlen(pass) == strlen(url_aux[0]) - 2 - strlen(user)) {
			fprintf(stderr,"usage: Declare a password\n");
			exit(1);
		}
		printf("PASS: %s\n",pass);
	}

	char** url_aux_2 = malloc(3 * sizeof(char *));
	size = 0;

	if(strlen(user) == 0) {
		size = strlen(url_aux[0]);
		url_aux_2[0] = malloc(size);
		memcpy(url_aux_2[0], url_aux[0], size);
	} else {
		size = strlen(url_aux[0]);
		url_aux_2[0] = malloc(size);
		memcpy(url_aux_2[0], url_aux[0] + strlen(user) + strlen(pass) + 4, size);
	}
	

	url_aux_2[0][size] = '\0';

	char* host = malloc(strlen(url_aux_2[0]));
	memcpy(host, url_aux_2[0], strlen(url_aux_2[0]));
	strtok(host, "/");

	if(strlen(host) == strlen(url_aux_2[0])) {
		fprintf(stderr,"usage: Declare an host\n");
		exit(1);
	}
	printf("HOST: %s\n",host);

	size = strlen(host) + 1;
	char** url_aux_3 = malloc(3 * sizeof(char *));
	url_aux_3[0] = malloc(size);
	memcpy(url_aux_3[0], url_aux_2[0] + strlen(host)+ 1, size);
	url_aux_3[0][size] = '\0';

	char* path = malloc(strlen(url_aux_3[0]));
	memcpy(path, url_aux_3[0], strlen(url_aux_3[0]));


	if(strlen(path) == 0) {
		fprintf(stderr,"usage: Declare an path\n");
		exit(1);
	}
	printf("PATH: %s\n",path);

	struct hostent *h;
	
    if ((h=gethostbyname(host)) == NULL) {  
		herror("gethostbyname");
		exit(1);
	}

	char * ip = inet_ntoa(*((struct in_addr *)h->h_addr));
	
	int sockfd;
	struct sockaddr_in server_addr;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(21);		/*server TCP port must be network byte ordered */
    
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
    	/*send a string to the server*/
	
	if(strlen(user) == 0) {
		strcpy(user, "anonymous");
		user[strlen(user)] = '\0';
		strcpy(pass, " ");
		pass[strlen(pass)] = '\0';
	}
	printf("%s 0\n",user);
	printf("%s 1\n",pass);

	char* write_user = malloc(strlen(user) + 1);
	char* write_pass = malloc(strlen(pass) + 1);
	printf("---------------\n");
	sprintf(write_user, "%s\n", user);
	sprintf(write_pass, "%s\n", pass);
	write_user[strlen(write_user)] = '\0';
	write_pass[strlen(write_pass)] = '\0';	
	char* msg = malloc(4 * sizeof(char));
	int i = 0;
	for(i = 0; i < strlen(msg); i++) {
		msg[i] = 0;
	}
	int nBytes = 0;
	printf("%s 2\n",write_user);
	printf("%s 3\n",write_pass);
	tcflush(sockfd, TCIOFLUSH);


	tcflush(sockfd, TCIOFLUSH);
	do {

		read(sockfd, msg, 4);
		printf("%s\n",msg);
	} while(!(strcmp(msg, "220 ") == 0));

	printf("%s 4\n",write_user);
	printf("%s 5\n",write_pass);
	if (nBytes = write(sockfd, write_user, strlen(write_user)) != strlen(write_user)) {
		fprintf(stderr,"usage: Number of Bytes Written wrong\n");
		exit(1);	
	}

	do {

		read(sockfd, msg, 4);
		printf("%s\n",msg);
	} while(!(strcmp(msg, "331 ") == 0));
	printf("HERE");

	if (nBytes = write(sockfd, write_pass, strlen(write_pass)) != strlen(write_pass)) {
		fprintf(stderr,"usage: Number of Bytes Written wrong\n");
		exit(1);	
	}

	do {

		read(sockfd, msg, 4);

	} while(!(strcmp(msg, "230 ") == 0));

	printf("HERE");
	printf("%s\n",msg);
	close(sockfd);	
	
	free(msg);
	free(user);
	free(pass);
	free(host);
	free(path);

	for(i = 0; i < 3; i++) {
		free(url_aux[i]);
		free(url_aux_2[i]);
		free(url_aux_3[i]);
	}
	free(url_aux);
	free(url_aux_2);
	free(url_aux_3);
	free(url_init);
}
