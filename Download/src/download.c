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

#include "ftp.h"

char* processElementUntilChar(char* str, char chr) {
	// using temporary string to process substrings
	char* tempStr = (char*) malloc(strlen(str));

	// calculating length to copy element
	int index = strlen(str) - strlen(strcpy(tempStr, strchr(str, chr)));

	tempStr[index] = '\0'; // termination char in the end of string
	strncpy(tempStr, str, index);
	strcpy(str, str + strlen(tempStr) + 1);

	return tempStr;
}

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

	size = strlen(url_aux_2[0]) - strlen(host) + 1;
	char** url_aux_3 = (char **)malloc(3 * sizeof(char *));
	url_aux_3[0] = (char *)malloc(size);
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
	
	ftp ftp;
	connect_ftp(&ftp,ip,21);
    	/*send a string to the server*/
	
	if(strlen(user) == 0) {
		strcpy(user, "anonymous");
		user[strlen(user)] = '\0';
		strcpy(pass, "");
		pass[strlen(pass)] = '\0';
	}


	char* write_user = malloc(strlen(user) + 1);
	char* write_pass = malloc(strlen(pass) + 1);
	printf("---------------\n");
	sprintf(write_user, "USER %s\r\n", user);
	sprintf(write_pass, "PASS %s\r\n", pass);
	write_user[strlen(write_user)] = '\0';
	write_pass[strlen(write_pass)] = '\0';	



	int nBytes = 0;

	FILE* fp = fdopen(ftp.control_socket_fd, "r");
	write_Sock(&ftp,write_user);
	char* read_user = malloc(52);
	read_user = read_Sock(fp, "331 ");		
	printf("%s\n",read_user);


	write_Sock(&ftp,write_pass);
	char* read_pass = malloc(52);
	read_pass = read_Sock(fp, "230 ");	
	printf("%s\n",read_pass);

	char* write_pasv = malloc(6);
	sprintf(write_pasv, "PASV\r\n");
	write_Sock(&ftp,write_pasv);
	char* read_pasv = malloc(52);
	read_pasv = read_Sock(fp, "227 ");	
	printf("READ_PASV: %s",read_pasv);
	
//DAQUI
	// starting process information
	int ipPart1, ipPart2, ipPart3, ipPart4;
	int port1, port2;
	if ((sscanf(read_pasv, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ipPart1,
			&ipPart2, &ipPart3, &ipPart4, &port1, &port2)) < 0) {
		printf("ERROR: Cannot process information to calculating port.\n");
		return 1;
	}

	// cleaning buffer
	memset(read_pasv, 0, sizeof(read_pasv));

	// forming ip
	if ((sprintf(read_pasv, "%d.%d.%d.%d", ipPart1, ipPart2, ipPart3, ipPart4))
			< 0) {
		printf("ERROR: Cannot form ip address.\n");
		return 1;
	}

	// calculating new port
	int portResult = port1 * 256 + port2;

	printf("IP: %s\n", read_pasv);
	printf("PORT: %d\n", portResult);


	if ((ftp.data_socket_fd = connect_Sock(read_pasv, portResult)) < 0) {
		printf(
				"ERROR: Incorrect file descriptor associated to ftp data socket fd.\n");
		return 1;
	}


//Ate aqui

	char* write_retr = malloc(6 + strlen(path));
	sprintf(write_retr, "RETR /%s\r\n", path);
	write_Sock(&ftp,write_retr);
	char* read_retr = malloc(52);
	read_retr = read_Sock(fp, "150 ");
	printf("%s\n",read_retr);

	char* element = (char*) malloc(strlen(url_aux_3[0]));
	int startPath = 1;
	while (strchr(url_aux_3[0], '/')) {
		element = processElementUntilChar(url_aux_3[0], '/');

		if (startPath) {
			startPath = 0;
			strcpy(path, element);
		} else {
			strcat(path, element);
		}

		strcat(path, "/");
	}

	char* filename = malloc(strlen(url_aux_3[0]));
	strcpy(filename, url_aux_3[0]);

//DAQUI
	FILE* file;
	int bytes;

	if (!(file = fopen(filename, "w"))) {
		printf("ERROR: Cannot open file.\n");
		return 1;
	}

	char buf[1024];
	while ((bytes = read(ftp.data_socket_fd, buf, sizeof(buf)))) {
		if (bytes < 0) {
			printf("ERROR: Nothing was received from data socket fd.\n");
			return 1;
		}

		if ((bytes = fwrite(buf, bytes, 1, file)) < 0) {
			printf("ERROR: Cannot write data in file.\n");
			return 1;
		}
	}

	fclose(file);
close(ftp.data_socket_fd);


	char disc[1024];
	char* read_aux = malloc(52);
	read_aux = read_Sock(fp, "226 ");
	printf("%s\n",read_aux);

	sprintf(disc, "QUIT\r\n");
	write_Sock(&ftp,disc);
	char* read_quit = malloc(52);
	read_quit = read_Sock(fp, "221 ");
	printf("%s\n",read_quit);

	if (ftp.control_socket_fd)
close(ftp.control_socket_fd);
//ATE AQUI


	free(write_pasv);
	free(user);
	free(pass);
	free(write_user);
	free(write_pass);
	free(host);
	free(path);
	int i = 0;
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
