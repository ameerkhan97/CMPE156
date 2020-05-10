//Ameer Khan
//CMPE 156
//Assignment #3
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
#include <pthread.h>
#include <libgen.h>

//struct for server data
typedef struct{
	int *port;
    char **ip;
} server;
server *serverdata;

//struct for thread data
typedef struct{
	int offset;
	int chunk;
	int id;
    char *filename;
} thread;

//fucntion for pthread_create
void *createthread(void *data){

    //initialize variables
	thread *filecontents = (thread *) data;
	struct sockaddr_in serv_addr;
	int id = filecontents->id;
	int sockfd, n;
	char compare;
	char *request = calloc(filecontents->chunk, 1);
    char *base1;
    base1 = basename(filecontents->filename);

    //opens socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "ERROR opening socket\n");
		exit(1);
	}

    serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverdata->port[id]);
	serv_addr.sin_addr.s_addr = inet_addr(serverdata->ip[id]);

    //establish connection to server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		fprintf(stderr, "ERROR connecting\n");
		exit(1);
	}

    //error handling for writing filecontents
	if ((write(sockfd, filecontents->filename, strlen(filecontents->filename))) < 0){
		fprintf(stderr, "ERROR writing");
		exit(1);
	}


    //checks to see if file content was requested
	if ((read(sockfd, &compare, 1)) > 0){
		if (strcmp(&compare, "content") == 0){
			fprintf(stderr, "ERROR opening file\n");
			exit(1);
		}
	}

    //writes the offset and chunks of each thread needed
	write(sockfd, "content", 1);
	int offsettemp = htonl(filecontents->offset);
	int chunktemp = htonl(filecontents->chunk);

	if ((write(sockfd, &offsettemp, sizeof(offsettemp))) < 0){
		fprintf(stderr, "ERROR writing\n");
		exit(1);
	}

	if ((write(sockfd, &chunktemp, sizeof(chunktemp))) < 0){
		fprintf(stderr, "ERROR writing\n");
		exit(1);
	}

    //reads in contents from file from server and outputs to textfile
	while ((n = read(sockfd, request, sizeof(request))) > 0){
		FILE *file;
		file = fopen(base1, "a");
		fwrite(request, 1, n, file);
        fclose(file);
	}
	pthread_exit(0);
}

int main(int argc, char *argv[]){

    //initialize variables
	int sockfd, n;
	int filesize, count;
	int rows = 128;
	int col = 100;
	char **token = malloc(sizeof(rows));
	char compare;
	struct sockaddr_in serv_addr;

    //malloc server information 
	serverdata = malloc(sizeof(server));
	serverdata->ip = malloc(sizeof(char));
	serverdata->port = malloc(sizeof(int));

    //checks to see if valid arguments were inputted
	if (argc != 4){
		fprintf(stderr, "Invalid, the input should be <server_filecontents.text> <num-connections> <filename>\n");
		exit(1);
	}
	int connections = atoi(argv[2]);

    //opens the first argument which has the server ip and port information
	FILE *file;
	file = fopen(argv[1],"r");
	if (file == NULL){
		fprintf(stderr, "ERROR opening %s file\n", argv[1]);
		exit(1);
	}


	for (int i = 0; 1; i++){
		token[i] = malloc(col);
		if (fgets(token[i], col-1, file) == NULL){
			break;
		}
		count++;
	}
	fclose(file);

	if (connections > count){
		connections = count;
	}

	for (int i = 0; i < connections; i++){
		serverdata->ip[i] = strtok(token[i], " ");
		serverdata->port[i] = atoi(strtok(NULL, " "));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(serverdata->port[i]);
		serv_addr.sin_addr.s_addr = inet_addr(serverdata->ip[i]);
	}

    //opens socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "ERROR opening socket\n");
		exit(1);
	}

    //establish connection to server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		fprintf(stderr, "ERROR connecting\n");
		exit(1);
	}

    //checks to see if file size was requested
	write(sockfd, argv[3], strlen(argv[3]));
	if ((read(sockfd, &compare, 1)) > 0){
		if (strcmp(&compare, "content") == 0){
			fprintf(stderr, "ERROR opening file\n");
			exit(1);
		}
	}
	
    //prints out file size that was requested on client side
	write(sockfd, "size", 1); 
	read(sockfd,&n,sizeof(n));
	filesize = ntohl(n);
	printf("Filesize: %d Bytes\n", filesize);

    //thread initializations
	pthread_t threadid[connections];
	thread *data[connections];

    //iteriates through the number of connections in order to calculate offset and chunks and creates the threads
	for (int i = 0; i < connections; i++){
		data[i] = malloc(sizeof(thread));
		data[i]->filename = argv[3];
		data[i]->id = i;

		if (i == (connections - 1)){
			data[i]->offset = i * (filesize/connections);
			data[i]->chunk = filesize/connections + ((((filesize*connections) - (filesize))/connections)*connections);
		} else {
			data[i]->offset = i * (filesize/connections);
			data[i]->chunk = filesize/connections;
		}

		int creation = pthread_create(&threadid[i], NULL, createthread, (void*) data[i]);
		if (creation != 0){
			fprintf(stderr, "ERROR creating threadid: %d", i);
			exit(1);
		}
		pthread_join(threadid[i], NULL);
	}

    close(sockfd);
}