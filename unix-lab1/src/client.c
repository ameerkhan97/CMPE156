//Ameer Khan
//CMPE 156
//Assignment #1
//client file

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

//function to find indices of the url
int findindex(char *s, char c){
	for (int i = 0; i < strlen(s); i++){
		if (s[i] == c){
			return i;
		}
	}
	return -1;
}

int main(int argc, char *argv[]){

	int sockfd, n, port, ip;
	char *url;
	char request[256];
	struct sockaddr_in serv_addr;
	struct hostent *server;
	FILE *file;

	//checks to see correct number of arguments are input by user
	if (argc < 3 || argc > 4){
		error("Invalid, the input should be: (ip address) (url) (-h which is optional)\n");
	}

	//set each argument to specified variable
	if (argc == 3){
		ip = atoi(argv[1]);
		url = argv[2];
		strcpy(request,"GET "); 
	} else if (argc == 4) {
		ip = atoi(argv[1]);
		url = argv[2];
		if (strcmp(argv[3],"-h") != 0){ 
			error("ERROR, the fourth argument needs to be -h\n");
		}
		strcpy(request,"HEAD ");
	}

	char host[strlen(url)];
	char path[strlen(url)];
	char portandpath[strlen(url)];
	char temp[strlen(url)];
	strcpy(portandpath, url);
	int colon = findindex(portandpath,':'); 
	int slash = findindex(portandpath, '/');

	//parses through url to find host, port and path if present
	if (colon != -1 && slash != -1){ //host, port and path all present
		strncpy(host, portandpath, colon);
		strncpy(temp, portandpath+colon+1, slash-colon-1);
		strncpy(path, portandpath+slash, strlen(url)-slash);
		host[colon] = '\0';
		temp[slash-colon-1] = '\0';
		port = atoi(temp);
		path[strlen(url)-slash] = '\0';
	} else if (colon == -1 && slash != -1){ //host and path present only
		strncpy(host, portandpath, slash);
		strncpy(path, portandpath+slash, strlen(url)-slash);
		host[slash] = '\0';
		port = 80;
		path[strlen(url)-slash] = '\0';
	} else if (colon != -1 && slash == -1){ //host and port present only
		strncpy(host, portandpath, colon);
		strncpy(temp, portandpath+colon+1, strlen(url)-colon-1);
		strcpy(path, "/");
		host[colon] = '\0';
		temp[slash-colon-1] = '\0';
		port = atoi(temp);
	} else if (colon == -1 && slash == -1){ //host present only
		strcpy(host, portandpath);
		port = 80;
		strcpy(path, "/");
	} else {
		error("ERROR, the third argument is the url which is in the format: hostname:port/pathname\n");
	}

	//concatenate request into valid GET or HEAD command
	strcat(request, path);
	strcat(request, " HTTP/1.1\r\nHost: ");
	strcat(request, host);
	strcat(request, "\r\n\r\n");

	//opens socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		error("ERROR opening socket\n");
	}

	//checks host is valid
	if ((server = gethostbyname(host)) == NULL){
		error("ERROR, no such host\n");
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

	//write HTTP request to server
	if (write(sockfd, request, sizeof(request)) < 0){
		error("ERROR writing to socket\n");
	}

	//opens output file
	if (argc == 3){
		file = fopen("output.dat","w");
	}

	//reads in HTTP request to output file or output screen depending on how many arguments
	while (1){
		n = read(sockfd, request, 255);
		if (n == 0){
			break;
		}
		if (n < 0){
			error("ERROR reading from socket\n");
		}
		request[n] = '\0'; 
		if (argc == 3) {
			fprintf(file,"%s",request); 
		} else if (argc == 4) {
			printf("\n%s", request);
		}
	}

	//closes file and socket
	if (argc == 3){
		fclose(file);
	}
	close(sockfd);
}
