//Ameer Khan
//CMPE 156
//Assignment #2
//client file

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

//error function for connecting to server
void error(const char *msg){
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]){

	int sockfd, n, port, ip;
	char request[256];
	struct sockaddr_in serv_addr;
	struct hostent *server;

	//checks to see correct number of arguments are input by user
	if (argc != 3){
		error("Invalid, the input should be: (ip address) (port number)\n");
	} else {
		ip = atoi(argv[1]);
		port = atoi(argv[2]);
	}

	//opens socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		error("ERROR opening socket\n");
	}

	//checks ip is valid
	if ((server = gethostbyname(argv[1])) == NULL){
		error("ERROR, no such ip\n");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	//checks if ip address is valid
	if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0){
		error("ERROR, ip address is invalid\n");
	}

	//establish connection to server
	if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		error("ERROR connecting");
	}


	//sends commands to server
	while (1){
		//prints prompt, clears buffer and reads in the command
		printf("client $ ");
		bzero(&request, sizeof(request));
		fgets(request, 255, stdin);

		//terminates client when exit is entered
		if (strcmp(request, "exit\n") == 0){
			n = write(sockfd, "EOF", strlen("EOF"));
			close(sockfd);
			exit(0);
		}

		//sends command to server
		if (write(sockfd, request, sizeof(request)) < 0){
			error("ERROR writing to socket\n");
		}

		//reads in command from server
		bzero(&request, sizeof(request));
		if (read(sockfd, request, 255) < 0){
			error("ERROR reading from socket\n");
			break;
		}
		
		printf("%s\n", request);
	}

	close(sockfd);
}