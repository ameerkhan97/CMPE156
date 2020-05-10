//Ameer Khan
//CMPE 156
//Final Project
//server file

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <pthread.h>
#include <ctype.h> 

//variables for the various commands
char connect1[] = "/connect ";
char list1[] = "/list";
char listspace[] = "";
char wait1[] = "/wait";

//struct for client usernames
typedef struct Client client;
struct Client{
    int socket;
    char username[20];
    int online;
    int save;
};

//struct client variables
const char *name = "Clients";
const int SIZE = 100 * sizeof(struct Client);
struct Client *clientname;
int clienttotal;

//functions
void *handle_client(int);
char *setup_client(char buffer[], int sock);
void handle_messages(char *username, char buffer[],int sock);
int connectcommand(char buffer[], char command[], int size);
int sendtoclient(char username[25], char message[256]);
int compare(char str1[], char str2[]);
int duplicate_client(char[]);


//handles communication with client once connection is established
void *handle_client(int sock){
    char buffer[256];
    char* username = setup_client(buffer, sock);
    handle_messages(username, buffer, sock);
    pthread_exit(0);
}

//sets up the client with provided username
char *setup_client(char buffer[], int sock){

    //allocate memory to username
    char *username = malloc(256 * sizeof(char));

    while(1){

        //clear buffer and read buffer
        bzero(buffer, 256);
        read(sock, buffer, 255);
        buffer[strlen(buffer)] = '\0';
        strcpy(username, buffer);
        fflush(stdout);

        //checks for null buffer otherwise returns username of client 
        if (strlen(buffer) <= 0){
          pthread_exit(0);
        } else if (duplicate_client(buffer)){
            printf("Username already exists: %s\n", buffer);
            bzero(buffer,256);
            write(sock, "Invalid: username already exists, please enter a new one below\n", 256);
            fflush(stdout);
        } else {
            printf("Client ID Created: %s\n", buffer);
            return username;
        }
    }
}

//handles messages supplied by the client
void handle_messages(char *username, char buffer[], int sock) {
    int n, i;
    char otherclient[25], message_to_send[256];
    int flag = 0;

    while(1) {

        //clear buffer, read it and print to stdout
        bzero(buffer, 255);
        n = read(sock, buffer, 256);
        printf("%s\n", buffer );

        //if /quit command entered, exit the program
        if (strncmp(buffer, "/quit", 5) == 0){
            write(sock,"Quit Program.\n",256);
            for (i = 0; i < clienttotal; i++){
                if (strcmp(username, clientname[i].username) == 0){
                    int pos = i;
                      for (int j=pos; j<clienttotal; j++){
                        memmove(clientname[j].username, clientname[j+1].username, 20);
                      }

                }
            }
            break;
        }

        //connect command
        if (connectcommand(buffer, connect1, 9)){

            //connects the client
            clientname[clienttotal].socket = sock;
            clientname[clienttotal].online = 0;
            sprintf(((struct Client *)clientname)[clienttotal].username, "%s", username);
            clienttotal = clienttotal + 1;
            fflush(stdout);

            //if username does not exist, print message and go back to info state
            char str[255];
            buffer[strlen(buffer)-1] = '\0';
            strcpy(otherclient, buffer+9);
            if(!sendtoclient(otherclient, str)){
               flag = 1;
               printf("No such user waiting named %s\n", otherclient);
               write(sock, "Invalid: username could not be found\n", 255);
               fflush(stdout);
               for (i = 0; i < clienttotal; i++){
                  if(strcmp(username, clientname[i].username) == 0){
                     int pos = i;
                     for(int j = pos; j < clienttotal; j++){
                        memmove(clientname[j].username, clientname[j+1].username, 20);
                      }
                  }
               }
               clienttotal--;
            }

            //if username does exist, connect to the client and enter chat state
            if (flag == 0){
              sprintf(str, "Connected to %s\n", otherclient);
              write(sock, str, 255);

              //send message to other client to let them know to enter chat state
              char return_message1[256];
              sprintf(return_message1, "Connection from %s\n> ", username);
              sendtoclient(otherclient, return_message1);
            }

            while (1){

                //flag to leave chat state if username that you are connecting to does not exist
                if (flag == 1){
                  flag = 0;
                  break;
                }

                while (1){

                    //clear buffer and read it 
                    bzero(buffer,255);
                    n = read(sock,buffer,256);

                    char return_message[256];
                    buffer[strlen(buffer)-1] = '\0';

                    //if control c or /quit command entered, leave chat state
                    if (strncmp(buffer, "exit", 4) == 0 || strncmp(buffer, "/quit", 5) == 0){
                        sprintf(return_message, "%s\n> ", buffer);
                        sendtoclient(otherclient, return_message);

                        char leave[255];
                        sprintf(leave, "Left Conversation with %s\n", otherclient);
                        write(sock, leave, 256);
                        fflush(stdout);

                        //remove client from connections
                        for (i = 0; i < clienttotal; i++){
                            if(strcmp(username, clientname[i].username) == 0){
                                int pos = i;
                                for(int j = pos; j < clienttotal; j++){
                                    memmove(clientname[j].username, clientname[j+1].username, 20);
                                }
                            }
                        }
                        clienttotal--;
                        break;
                    }

                    //sends message to other client
                    sprintf(return_message, "%s: %s\n> ", username, buffer);
                    sendtoclient(otherclient, return_message);
                    fflush(stdout);
                }
                break;
            }
        }

        //wait command
        else if (compare(buffer, wait1)){

            //connects the client
            write(sock,"Waiting for a connection\n",256);
            clientname[clienttotal].socket = sock;
            clientname[clienttotal].online = 1;
            int save = clienttotal;
            sprintf(((struct Client *)clientname)[clienttotal].username, "%s", username);
            clienttotal = clienttotal + 1;
            fflush(stdout);

            while(1) {

                //clear buffer and read it
                bzero(buffer, 255);
                n = read(sock, buffer, 256);

                //if client crashes and exits, remove from wait state
                if (strlen(buffer) <= 0){
                    for (i = 0; i < clienttotal; i++){
                        if (strcmp(username, clientname[i].username) == 0){
                            int pos = i;
                            for (int j = pos; j < clienttotal; j++){
                                memmove(clientname[j].username, clientname[j+1].username, 20);
                            }
                        }
                    }
                    clienttotal--;
                    printf("%d\n", clienttotal );
                    break;
                }

                //if control c was entered, leave wait state
                if (strncmp(buffer, "stop", 4) == 0){
                    write(sock,"Stopped waiting.\n",256);
                    for (i = 0; i < clienttotal; i++){
                        if (strcmp(username, clientname[i].username) == 0){
                            int pos = i;
                            for (int j=pos; j<clienttotal; j++){
                                memmove(clientname[j].username, clientname[j+1].username, 20);
                            }

                        }
                    }
                    clienttotal--;
                    printf("%d\n", clienttotal );
                    break;
                }

                //if /quit was entered, exit the program
                if (strncmp(buffer, "/quit", 5) == 0){
                    write(sock,"Stopped waiting.\n",256);
                    for (i = 0; i < clienttotal; i++){
                        if (strcmp(username, clientname[i].username) == 0){
                            int pos = i;
                            for (int j=pos; j<clienttotal; j++){
                                memmove(clientname[j].username, clientname[j+1].username, 20);
                            }

                        }
                    }
                    clienttotal--;
                    printf("%d\n", clienttotal );
                    break;
                }

                //checks if connection received from another client, if so enter chat state
                if (strncmp(buffer, "Connection", 10) == 0){
                    clientname[save].online = 0;
                    char str[255];
                    char *ptr;

                    //gets id of the other client
                    buffer[strlen(buffer)-1] = '\0';
                    ptr = strtok(buffer, " ");
                    ptr = strtok(NULL, " ");
                    ptr = strtok(NULL, " ");
                    strcpy(otherclient, ptr);

                    while(1) {

                        //clears buffer and reads it
                        bzero(buffer, 255);
                        n = read(sock,buffer, 256);

                        char return_message[256];
                        buffer[strlen(buffer)-1] = '\0';

                        //if control c or /quit command entered, leave chat state
                        if (strncmp(buffer, "exit", 4) == 0 || strncmp(buffer, "/quit", 5) == 0){
                            sprintf(return_message, "%s\n> ", buffer);
                            sendtoclient(otherclient, return_message);

                            char leave[255];
                            sprintf(leave, "Left Conversation with %s\n", otherclient);
                            write(sock, leave, 256);
                            fflush(stdout);

                            //remove client from connections
                            for (i = 0; i < clienttotal; i++){
                                if (strcmp(username, clientname[i].username) == 0){
                                    int pos = i;
                                    for (int j = pos; j < clienttotal; j++){
                                        memmove(clientname[j].username, clientname[j+1].username, 20);
                                    }

                                }
                            }
                            clienttotal--;
                            break;
                        }

                        //sends message to other client
                        sprintf(return_message, "%s: %s\n> ", username, buffer);
                        sendtoclient(otherclient, return_message);
                        fflush(stdout);
                    }
                    break;
                }
            }

        }

        //list command
        else if(compare(buffer, list1)) {
            write(sock, listspace, 255);
            int num = 1;

            //prints clients in the wait state
            for (i = 0; i < clienttotal; i++){
              if (clientname[i].online == 1){
                char str[255];
                sprintf(str, "%d) %s\n", num++, clientname[i].username);
                write(sock, str, 255);
              }
            }
            fflush(stdout);
        }
    }
}


//send message to specified client
int sendtoclient(char username[25], char message[256]){
    for(int i = 0; i < clienttotal; i++){
        if(strcmp(username, clientname[i].username) == 0){
            write(clientname[i].socket, message, 255);
            return 1;
        }
    }
    return 0;
}

//checks to see if /connect command was entered
int connectcommand(char buffer[], char command[], int size){
    char connectcommand[10];
    strncpy(connectcommand, buffer, size);
    return(compare(connectcommand, command));
}

//compare buffers
int compare(char str1[], char str2[]){
    for(int i = 0; i < strlen(str1)-1; i++){ 
        if(str1[i] != str2[i]){
            return 0;
        }
    }
    return 1;
}

//checks for duplicate clients
int duplicate_client(char buffer[]){
    for(int i = 0; i < clienttotal; i++){
        if(strcmp(((struct Client *)clientname)[i].username, buffer) == 0){
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]){

  //intialize variables
  int sockfd, newsockfd, port, memory;
  int id = 0;
  char buffer[256];
  pthread_t threads[100];
  struct sockaddr_in serv_addr, cli_addr;
  socklen_t len;

  //checks to see if valid arguments were inputted
  if (argc != 2){
    fprintf(stderr, "Invalid, the input should be <port number>\n");
    exit(1);
  }
  port = atoi(argv[1]);

  //memory allocation
  memory = shm_open(name, O_CREAT | O_RDWR, 0666);
  ftruncate(memory, SIZE);
  clientname = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0);
  if(clientname ==  MAP_FAILED) {
    printf("Map failed\n");
    return -1;
  }

  //opens socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    fprintf(stderr, "ERROR opening socket\n");
    exit(1);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  //binds socket to port number, then listens for incoming connections
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
    fprintf(stderr, "ERROR on binding\n");
    exit(1);
  }

  listen(sockfd, 5);
  len = sizeof(cli_addr);

  while(1) {

    //accepts connection
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &len);

    //error handling for checking if connection was accepted
    if (newsockfd < 0){
      fprintf(stderr, "ERROR on accepting\n");
      exit(1);
    }

    //creates thread for handling clients
    pthread_create(&threads[id], NULL, handle_client, (void *) newsockfd);
  } 

  //closes socket
  close(sockfd);
}