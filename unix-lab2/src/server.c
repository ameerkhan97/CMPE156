//Ameer Khan
//CMPE 156
//Assignment #2
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

//error function for connecting to server
void error(const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){

    int sockfd, n, port, newsockfd;
    char request[2048];
    char *command;
    struct sockaddr_in serv_addr;
    FILE *file;

    //checks to see correct number of arguments are input by user
    if (argc != 2){
        error("Invalid, the input should be: (port number)\n");
    } else {
        port = atoi(argv[1]);
    }

    //opens socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        error("ERROR opening socket\n");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    
    //binds socket to port number
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        error("ERROR on binding");
    }

    //listens for incoming connection from a client and accepts if valid
    listen(sockfd, 5);
    newsockfd = accept(sockfd, (struct sockaddr *) NULL, NULL);

    //checks to see if connection is valid
    if (newsockfd < 0){
        error("ERROR on accepting");
    } else {
        printf("Client is connected\n");
    }

    //sends output of command requested by the client
    while (1){
        //clears buffer and reads in the request from the client
        bzero(&request, sizeof(request));
        n = read(newsockfd, request, 2047);

        //error checking for reading in request from client
        if (n < 0){
            error("ERROR reading from socket\n");
        }

        //listens again for client if disconnected 
        if (strcmp(request, "EOF") == 0){
            printf("Client has disconnected\n");
            listen(sockfd, 5);
            newsockfd = accept(sockfd, (struct sockaddr *) NULL, NULL);
            if (newsockfd < 0){
                error("ERROR on accepting\n");
            } else {
                printf("Client is connected\n");
            }
            continue;
        }

        //reads in output of command recieved from client into a file
        file = popen(request, "r");
        if (file == NULL){
            error("ERROR, file is null\n");
        } else {
            printf("%s", request);
        }

        //reads in output of the command and prints error message if command is invalid
        command = fgets(request, 2047, file);
        if (command == NULL){
            n = write(newsockfd, "Command not found\n", strlen("Command not found\n"));
            printf("Command Invalid: %s", request);
        }

        //writes ouput of command requested to the client
        while (command != NULL){
            n = write(newsockfd, request, strlen(request));
            if (n < 0){
                error("ERROR writing to socket\n");
            }
            command = fgets(request, 2047, file);
        }

        pclose(file);
    }

    close(sockfd);
}