//Ameer Khan
//CMPE 156
//Final Project
//client file

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>


//global variables
int sock;
int exitthread = 0;
char quitting[256] = "/quit";
char username[256];
sig_atomic_t volatile g_running = 1;

//compare buffers
int compare(char str1[], char str2[]){
    int i;
    for(i = 0; i < strlen(str1)-1; i++){
        if(str1[i] != str2[i]){
            return 0;
        }
    }
    return 1;
}

//handles control c signal
void sig_handler(int signum)
{
  if (signum == SIGINT)
    g_running = 0;
}

//writes message to server in order to send to another client
void *writetoserver(){
    char buffer[256];
    while(1){

        //prints client names> buffer output to stdout 
        sleep(1);
        printf("%s> ",username);

        //checks if control c was entered
        while (g_running == 0){
            bzero(buffer, 256);
            write(sock, "exits", 5);
            write(sock, "stops", 5);
            g_running = 1;
        }

        //clears buffer and then get the buffer
        bzero(buffer,256);
        fgets(buffer,255,stdin);

        //checks to see if /quit was entered and if so quit the program
        if(buffer[0] == '\n'){
            continue;
        } 
        //write to socket
        write(sock,buffer,strlen(buffer));

        if(compare(buffer, quitting)){
            exitthread = 1;
            close(sock);
            pthread_exit(0);
        }

        //write to socket
        //write(sock,buffer,strlen(buffer));
    }
}

//reads message from server that was sent by another client
void *readfromserver(){
    char buffer[256];
    while(1){

        //reads buffer output
        fflush(stdout);
        read(sock, buffer, 255);

        //checks to see if the string Connection was sent
        if (strncmp(buffer, "Connection", 10) == 0){
            write(sock, buffer, strlen(buffer)-2);
        }

        //checks to see if the string exit was sent
        if (strncmp(buffer, "exit", 4) == 0){
            write(sock, buffer, strlen(buffer));
        }

        if (strncmp(buffer, "/quit", 5) == 0){
            write(sock, buffer, strlen(buffer));
        }

        //prints buffer recieved to screen
        if(strlen(buffer) != 0){
            printf("%s",buffer);
        }

        //clears buffer
        bzero(buffer,256);

        //exits the thread if /quit was entered
        if(exitthread){
            pthread_exit(0);
        }
    }
}

int main(int argc, char *argv[]){

    //initialize variables
    int port, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    pthread_t writeid;
    pthread_t readid;

    signal(SIGINT, &sig_handler);
    strcpy(username, argv[3]);

    //checks to see if valid arguments were inputted
    if (argc != 4){
        fprintf(stderr, "Invalid, the input should be <ip address> <port number> <client name>\n");
        exit(1);
    }
    port = atoi(argv[2]);

    //opens socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        fprintf(stderr, "ERROR opening socket\n");
        exit(1);
    }

    //checks if host is valid
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    //establish connection to server
    if (connect(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        fprintf(stderr, "ERROR connecting\n");
        exit(1);
    }

    //writes client name to stdout on first line
    n = write(sock, username, strlen(username));

    //error checking for writing to the socket
    if (n < 0){
        fprintf(stderr, "ERROR writing to socket\n");
        exit(1);
    }

    //creates two threads for reading and writing
    pthread_create(&writeid, NULL, writetoserver, NULL);
    pthread_create(&readid, NULL, readfromserver, NULL);
    
    //joins the two threads created for reading and writing
    pthread_join(writeid, NULL);
    pthread_join(readid, NULL);
}