//Ameer Khan
//CMPE 156
//Assignment #5
//proxy server file

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
#include <pthread.h>
#include <libgen.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/timeb.h>


//struct for socket variables
typedef struct {
    int client;
    int proxy;
    int end;
} thread;

//intialize global variables
char message[2048];
char **bansite = NULL;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

//intialize helper functions
int goodsite(char *url);
void *client_browser(void *data);
void *browser_client(void *data);
int connectbrowser(int client, struct sockaddr_in cli_addr, struct sockaddr_in serv_addr);
void logfile(struct sockaddr_in cli_addr, char *request, char *url, char *method, char *code, int size);

//function to check whether the site being requested is blocked or not
int goodsite(char *url){
    for (int i = 0; bansite[i] != NULL; i++){
        if ((strcasecmp(url, bansite[i])) == 0){
            return 0;
        }
    }
    return 1;
}

//function to get the request information
void *client_browser(void *data){
    //intializes variables
    int n;
    char buffer[40000], request[100], url[255], method[100], *host;
    thread *t = (thread *) data;
    
    //read in content of request untill you are done
    while ((!(t->end) && (n = recv(t->client, buffer, 2048, 0)) > 0)){
        sscanf(buffer,"%s %s %s",request, url, method);
        //check to see if valid request
        if (((strncmp(request, "GET", 3) == 0) || (strncmp(request, "HEAD", 4) == 0)) && ((strncmp(method, "HTTP/1.1", 8) == 0) || (strncmp(method, "HTTP/1.0", 8) == 0)) && (strncasecmp(url, "http://", 7) == 0)){
            host = strtok(url, "//");
            host = strtok(NULL, "/");
            //check if host is blocked or not
            if (goodsite(host)){
                if (!(t->end)){
                    send(t->proxy, buffer, n, 0);
                }
            //if blocked, send 403 forbidden url message
            } else {
                strcpy(message, "HTTP/1.1 403 Forbidden URL\r\n\r\n");
                printf("Sending ERROR CODE 403 Forbidden URL\n");
                send(t->client, message, strlen(message), 0);
                break;
            }
        //if request is not valid, send 501 not implemented message
        } else {
            strcpy(message, "HTTP/1.1 501 Not implemented\r\n\r\n");
            printf("Sending ERROR CODE 501 Not implemented\n");
            send(t->client, message, strlen(message), 0);
            break;
        }
    }
    //flag to close connection
    t->end = 1;
    close(t->client);
    close(t->proxy);
    free(t);
}

//function to get the html body page
void *browser_client(void *data){
    //initializes variables
    int n;
    char buffer[2048];
    thread *t = (thread *) data;
    char *temp;

    //read in content of html page until you are done
    while ((!(t->end)) && ((n = recv(t->proxy, buffer, 2048, 0)) > 0)){
        printf("\n%s\n", buffer);
        //send content of html body page
        if (!(t->end)){ 
            send(t->client, buffer, n, 0);
        }     
    }
    //flag to close connection
    t->end = 1;
}

//function to connect to host
int connectbrowser(int client, struct sockaddr_in cli_addr, struct sockaddr_in serv_addr){
    //intializes variables
    int browsersockfd, browserport, n;
    char buffer[40000], request[100], url[255], method[100], *host;
    char logurl[255];
    struct sockaddr_in browser_addr;
    struct hostent *browser_host;

    //find client ip and proxy ip for forwarding header
    char forward[80];
    sprintf(forward, "Forwarded: for=<%s>; proto=http; by=<%s>\n", inet_ntoa(cli_addr.sin_addr), inet_ntoa(serv_addr.sin_addr));

    //recieves the request buffer and adds the forwarded header
    if ((n = recv(client, buffer, 40000, MSG_PEEK)) > 0){
        char *newbuffer = (char*) malloc(strlen(buffer) + strlen(forward) + 1);
        strcpy(newbuffer, buffer);
        newbuffer[strlen(buffer) - 2] = '\0';
        strcat(newbuffer, forward);
        strcat(newbuffer, "\r\n");
        printf("%s\n",newbuffer);
        sscanf(newbuffer, "%s %s %s", request, url, method);
        strcpy(logurl, url);
        //check if request is valid
        if (((strncmp(request, "GET", 3) == 0) || (strncmp(request, "HEAD", 4) == 0)) && ((strncmp(method, "HTTP/1.1", 8) == 0) || (strncmp(method, "HTTP/1.0", 8) == 0)) && (strncasecmp(url, "http://", 7) == 0)){       
            printf("Recieving request for %s\n", logurl);
            browsersockfd = socket(AF_INET, SOCK_STREAM, 0);
            host = strtok(url, "//");
            host = strtok(NULL, "/");
            //check if host is valid
            if ((browser_host = gethostbyname(host)) == NULL){
                printf("ERROR for gethostbyname\n");
                close(browsersockfd);
                logfile(cli_addr, request, logurl, method, "400", 0);
                return -1;
            }
            //check if host is blocked or not
            if (!goodsite(host)){
                printf("Host: %s\n", host);
                logfile(cli_addr, request, logurl, method, "403", 0);
                return -2;
            }  
            printf("Host: %s\n", host);
            browserport = 80;
            
            bzero(&browser_addr, sizeof(browser_addr));
            browser_addr.sin_family = AF_INET;
            memcpy(&browser_addr.sin_addr.s_addr, browser_host->h_addr, browser_host->h_length);
            browser_addr.sin_port = htons(browserport);
            printf("IP Address of Host: %s\n", inet_ntoa(browser_addr.sin_addr));

            //connect to host
            if ((connect(browsersockfd, (struct sockaddr *) &browser_addr, sizeof(browser_addr))) < 0){
                printf("ERROR connecting\n");
                close(browsersockfd);   
                logfile(cli_addr, request, logurl, method, "400", 0); 
                return -1;
            }
            logfile(cli_addr, request, logurl, method, "200", n);
        } else {
            logfile(cli_addr, request, logurl, method, "501", 0);
        } 
    }
    return browsersockfd;
}

//function to log information to a .log file
void logfile(struct sockaddr_in cli_addr, char *request, char *url, char *method, char *code, int size){
    //lock thread in order to log file
    pthread_mutex_lock(&m);
    FILE *file = fopen("access.log", "a");

    //intializes variables
    char buffer[100];
    char append[100];
    struct timeb start;

    //find the date and time in specific format
    ftime(&start);
    strftime(buffer, 100, "%Y-%m-%dT%H:%M:%S", localtime(&start.time));
    sprintf(append, ".%03uPST", start.millitm);
    strcat(buffer, append);
    
    //print information to log file if file not null
    if (file) {
        fprintf(file, "%s ", buffer);
        fprintf(file, "%s ", inet_ntoa(cli_addr.sin_addr));
        fprintf(file, "\"%s %s %s\" ", request, url, method);
        fprintf(file, "%s ", code);
        fprintf(file, "%d\n", size);
    } else {
        printf("ERROR writing to log file\n");
    }

    //close file and unlock thread
    fclose(file);
    pthread_mutex_unlock(&m);
}

int main(int argc, char **argv){
    //initializes variables
    int sockfd, i, j;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    socklen_t len = sizeof(cli_addr);
    FILE *file;
    
    //checks to see if valid arguments were inputted
    if ((argc != 3)){
        fprintf(stderr, "Invalid, the input should be <port number> <forbidden-sites text file>\n");
        exit(1);
    }

    //check to see if forbidden text file is null or not
    file = fopen(argv[2], "r");   
    if (file == NULL){
        fprintf(stderr, "ERROR opening %s file\n", argv[2]);
        exit(1);
    }
    
    //allocate memory
    if ((bansite = calloc(200, sizeof(char *))) == NULL){
        fprintf(stderr, "ERROR for memory\n");
        exit(1);
    }
    
    //parse through forbidden text file
    for (i = 0; 1; i++){
        //allocate memory
        if ((bansite[i] = calloc(255, sizeof(char))) == NULL){
            fprintf(stderr, "ERROR for memory\n");
            exit(1);
        }
        if ((fgets(bansite[i], 255, file)) == NULL){
            free(bansite[i]);
            bansite[i] = NULL;
            break;
        }
        //remove end characters
        j = strlen(bansite[i]) - 1;
        while (j <= 0 && (bansite[i][j] == '\r' || bansite[i][j] == '\n')){
            j--;
        }
        bansite[i][j] = '\0';
    }
    fclose(file);
    
    //connect to client
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "ERROR opening socket\n");
        exit(1);
    }
    
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    
    //binds socket to port number, then listens for incoming connections
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        fprintf(stderr, "ERROR on binding\n");
        exit(1);
    }
    listen(sockfd, 10);
    
    //implement concurrency
    while (1){
        //initialize thread variables
        pthread_t id;
        thread *t;
        
        //allocate memory
        if ((t = calloc(1, sizeof(thread))) == NULL) {
            printf("ERROR allocating memory!\n\n");
        }

        //accept connection
        t->client = accept(sockfd, (struct sockaddr *) &cli_addr, &len);
        
        //connecting to host
        if((t->proxy = connectbrowser(t->client, cli_addr, serv_addr)) > 0) {   
            //create first thread for concurrency      
            int creation = pthread_create(&id, NULL, client_browser, (void *) t);
            if (creation != 0){
                fprintf(stderr, "ERROR creating thread\n");
                exit(1);
            }
            if ((creation = pthread_detach(id)) != 0) {
                fprintf(stderr, "ERROR detaching thread\n");
                exit(1);
            }
            //create second thread for concurrency
            creation = pthread_create(&id, NULL, browser_client, (void *) t);
            if (creation != 0){
                fprintf(stderr, "ERROR creating thread\n");
                exit(1);
            }
            if ((creation = pthread_detach(id)) != 0) {
                fprintf(stderr, "ERROR detaching thread\n");
                exit(1);
            }
        //send 400 bad request message
        } else if (t->proxy == -1) {
            strcpy(message, "HTTP/1.1 400 Bad Request\r\n\r\n");
            printf("Sending ERROR CODE 400 Bad Request\n");
            send(t->client, message, strlen(message), 0);
            close(t->client);
            free(t);
        //send 403 forbidden url message
        } else if (t->proxy == -2) {
            strcpy(message, "HTTP/1.1 403 Forbidden URL\r\n\r\n");
            printf("Sending ERROR CODE 403 Forbidden URL\n");
            send(t->client, message, strlen(message), 0);
            close(t->client);
            free(t);
        //send 501 not implemented message
        } else {
            strcpy(message, "HTTP/1.1 501 Not implemented\r\n\r\n");
            printf("Sending ERROR CODE 501 Not implemented\n");
            send(t->client, message, strlen(message), 0);
            close(t->client);
            free(t);
        }
    }
}