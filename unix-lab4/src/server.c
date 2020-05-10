//Ameer Khan
//CMPE 156
//Assignment #4
//server file

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
#include <libgen.h>

//function for finding file size
int fsize(FILE *file){
	int sz;
	fseek(file, 0L, SEEK_END);
	sz = ftell(file);
	return sz;
}

int main(int argc, char *argv[]){

    //initialize variables 
	int sockfd, newsockfd, n;
	int filesize, filesizetemp;
	int startbyte, offset, downloadsize, bytesent;
	char buffer[1000000];
	char filename[1000000];
	char compare;
    struct sockaddr_in serv_addr;
    struct sockaddr_in from;
    socklen_t fromlen;

    //checks to see if valid arguments were inputted
	if (argc != 2){
		fprintf(stderr, "Invalid, the input should be <port number>\n");
		exit(1);
	}

    //opens a socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr, "ERROR opening socket\n");
		exit(1);
	}

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));
	fromlen = sizeof(from);

    //binds socket to port number, then listens for incoming connections
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		fprintf(stderr, "ERROR on binding\n");
		exit(1);
	}

    //checks to see whether file size or file contents were requested
	while(1){

        //reads in the contents of filename requested
		if ((n = recvfrom(sockfd, filename, 1000000, 0, (struct sockaddr *) &from, &fromlen)) > 0){
			printf("Client is connected\n");
			filename[n] = 0;
		}
		printf("Name of File Requested: %s\n", filename);

        //opens file requested and sends the file size and file contents depending on what was requested
		FILE *file = fopen(filename,"r");
		if (file == NULL){
			fprintf(stderr, "ERROR opening file\n");
		} else {
			sendto(sockfd, "size", 1, 0, (struct sockaddr *) &from, fromlen);
			recvfrom(sockfd, &compare, 1, 0, (struct sockaddr *) &from, &fromlen);
			if (strncmp(&compare, "size", 1) == 0){
				filesize = fsize(file);
				filesizetemp = htonl(filesize);
				sendto(sockfd, &filesizetemp, sizeof(filesizetemp), 0, (struct sockaddr *) &from, fromlen);
                printf("File Size Requested\n");
			} else if (strncmp(&compare, "content", 1) == 0){
				recvfrom(sockfd, &startbyte, sizeof(startbyte), 0, (struct sockaddr *) &from, &fromlen);
				offset = ntohl(startbyte);
                printf("File Content Requested\n");
				printf("Offset: %d\n", offset);
				if (recvfrom(sockfd, &startbyte, sizeof(startbyte), 0, (struct sockaddr *) &from, &fromlen) > 0){
					downloadsize = ntohl(startbyte);
				}
				fseek(file, offset, SEEK_SET);
				bytesent = fread(buffer, 1, downloadsize, file);
				printf("Bytes Sent: %d\n", bytesent);
				sendto(sockfd, buffer, bytesent, 0, (struct sockaddr *) &from, fromlen);
				fclose(file);
			}
		}
		printf("Client has closed\n");
	}
}