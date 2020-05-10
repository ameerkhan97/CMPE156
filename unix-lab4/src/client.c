//Ameer Khan
//CMPE 156
//Assignment #4
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
	struct sockaddr_in from;
	unsigned int length;
	int id = filecontents->id;
	int sockfd, n;
	char compare;
	char *request = calloc(filecontents->chunk, 1);
    char *base1;
    base1 = basename(filecontents->filename);

    //opens socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr, "ERROR opening socket\n");
		exit(1);
	}

	bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverdata->port[id]);
	serv_addr.sin_addr.s_addr = inet_addr(serverdata->ip[id]);
	length = sizeof(struct sockaddr_in);

    //error handling for writing filecontents
	if ((sendto(sockfd, filecontents->filename, strlen(filecontents->filename), 0, (struct sockaddr *) &serv_addr, length)) < 0){
		fprintf(stderr, "ERROR writing");
		exit(1);
	}


    //checks to see if file content was requested
	if ((recvfrom(sockfd, &compare, 1, 0, (struct sockaddr *) &from, &length)) > 0){
		if (strcmp(&compare, "content") == 0){
			fprintf(stderr, "ERROR opening file\n");
			exit(1);
		}
	}

    //writes the offset and chunks of each thread needed
	sendto(sockfd, "content", 1, 0, (struct sockaddr *) &serv_addr, length);
	int offsettemp = htonl(filecontents->offset);
	int chunktemp = htonl(filecontents->chunk);

	if ((sendto(sockfd, &offsettemp, sizeof(offsettemp), 0, (struct sockaddr *) &serv_addr, length)) < 0){
		fprintf(stderr, "ERROR writing\n");
		exit(1);
	}

	if ((sendto(sockfd, &chunktemp, sizeof(chunktemp), 0, (struct sockaddr *) &serv_addr, length)) < 0){
		fprintf(stderr, "ERROR writing\n");
		exit(1);
	}

    //reads in contents from file from server and outputs to textfile
	while ((n = recvfrom(sockfd, request, 1000000, 0, (struct sockaddr *) &from, &length)) > 0){
		FILE *file;
		file = fopen(base1, "a");
		fwrite(request, 1, n, file);
        fclose(file);
		pthread_exit(0);
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
	struct sockaddr_in from;
	unsigned int length;

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

	//parses through server-info.txt
	for (int i = 0; 1; i++){
		token[i] = malloc(col);
		if (fgets(token[i], col-1, file) == NULL){
			break;
		}
		count++;
	}
	fclose(file);

	//opens socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr, "ERROR opening socket\n");
		exit(1);
	}

	//connects ip and port
	if (connections > count) {
		for (int i = 0; i < count; i++){
			serverdata->ip[i] = strtok(token[i], " ");
			serverdata->port[i] = atoi(strtok(NULL, " "));
			bzero(&serv_addr, sizeof(serv_addr));
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(serverdata->port[i]);
			serv_addr.sin_addr.s_addr = inet_addr(serverdata->ip[i]);
			length = sizeof(struct sockaddr_in);
		}
	} else {
		for (int i = 0; i < connections; i++){
			serverdata->ip[i] = strtok(token[i], " ");
			serverdata->port[i] = atoi(strtok(NULL, " "));
			bzero(&serv_addr, sizeof(serv_addr));
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(serverdata->port[i]);
			serv_addr.sin_addr.s_addr = inet_addr(serverdata->ip[i]);
			length = sizeof(struct sockaddr_in);
		}
	}

    //checks to see if file size was requested
    sendto(sockfd, argv[3], strlen(argv[3]), 0, (struct sockaddr *) &serv_addr, length);
	if ((recvfrom(sockfd, &compare, 1, 0, (struct sockaddr *) &from, &length)) > 0){
		if (strcmp(&compare, "content") == 0){
			fprintf(stderr, "ERROR opening file\n");
			exit(1);
		}
	}
	
    //prints out file size that was requested on client side
    sendto(sockfd, "size", 1, 0, (struct sockaddr *) &serv_addr, length);
    recvfrom(sockfd, &n, sizeof(n), 0, (struct sockaddr *) &from, &length);
	filesize = ntohl(n);
	printf("Filesize: %d Bytes\n", filesize);

    //thread initializations
	pthread_t threadid[count];
	thread *data[count];

    //iteriates through the number of connections in order to calculate offset and chunks and creates the threads
    if (filesize/connections > 65527){
    	int temp1 = 0;
    	for (int j = 0; j < 16; j++){
			for (int i = 0; i < connections; i++){
				data[i] = malloc(sizeof(thread));
				data[i]->filename = argv[3];
				data[i]->id = i;

				if (i == ((connections) - 1) && j == 15){
						data[i]->offset = (temp1) * (filesize/16);
						data[i]->chunk = filesize/16 + ((((filesize*16) - (filesize))/connections)*16);
				} else {
						data[i]->offset = (temp1++) * (filesize/16);
						data[i]->chunk = filesize/16;
				}

				int creation = pthread_create(&threadid[i], NULL, createthread, (void*) data[i]);
				if (creation != 0){
					fprintf(stderr, "ERROR creating threadid: %d", i);
					exit(1);
				}
				pthread_join(threadid[i], NULL);
			}
		}
	 } else if (connections > count){
	 	for (int i = 0; i < count; i++){
			data[i] = malloc(sizeof(thread));
			data[i]->filename = argv[3];
			data[i]->id = i;

			data[i]->offset = i * (filesize/connections);
			data[i]->chunk = filesize/connections;

			int creation = pthread_create(&threadid[i], NULL, createthread, (void*) data[i]);
			if (creation != 0){
				fprintf(stderr, "ERROR creating threadid: %d", i);
				exit(1);
			}
			pthread_join(threadid[i], NULL);
		}
		int temp = count;
		if (connections % count != 0){
				if (connections - count > count){
					for (int j = 0; j < connections/count; j++){
						for (int i = 0; i < count; i++){
							data[i] = malloc(sizeof(thread));
							data[i]->filename = argv[3];
							data[i]->id = i;
							int k;

							if (i == ((count) - 1) && j == connections/count - 1 ){
								data[i]->offset = (temp) * (filesize/connections);
								data[i]->chunk = filesize/connections + ((((filesize*connections) - (filesize))/connections)*connections);
								k = 0;
							} else {
								data[i]->offset = (temp++) * (filesize/connections);
								data[i]->chunk = filesize/connections;
							}

							int creation = pthread_create(&threadid[i], NULL, createthread, (void*) data[i]);
							if (creation != 0){
								fprintf(stderr, "ERROR creating threadid: %d", i);
								exit(1);
							}
							pthread_join(threadid[i], NULL);
							if (k = 0){
								break;
							}
						}
					}
				} else {
					for (int i = 0; i < connections - count; i++){
						data[i] = malloc(sizeof(thread));
						data[i]->filename = argv[3];
						data[i]->id = i;

						if (i == ((connections - count) - 1)){
							data[i]->offset = (temp) * (filesize/connections);
							data[i]->chunk = filesize/connections + ((((filesize*connections) - (filesize))/connections)*connections);
						} else {
							data[i]->offset = (temp++) * (filesize/connections);
							data[i]->chunk = filesize/connections;
						}

						int creation = pthread_create(&threadid[i], NULL, createthread, (void*) data[i]);
						if (creation != 0){
							fprintf(stderr, "ERROR creating threadid: %d", i);
							exit(1);
						}
						pthread_join(threadid[i], NULL);
					}
				}
		}
		temp = count;
		if (connections % count == 0){
				for (int j = 0; j < connections/count - 1; j++){
					for (int i = 0; i < count; i++){
						data[i] = malloc(sizeof(thread));
						data[i]->filename = argv[3];
						data[i]->id = i;

						if (i == ((count) - 1) && j == connections/count - 2 ){
							data[i]->offset = (temp) * (filesize/connections);
							data[i]->chunk = filesize/connections + ((((filesize*connections) - (filesize))/connections)*connections);
						} else {
							data[i]->offset = (temp++) * (filesize/connections);
							data[i]->chunk = filesize/connections;
						}

						int creation = pthread_create(&threadid[i], NULL, createthread, (void*) data[i]);
						if (creation != 0){
							fprintf(stderr, "ERROR creating threadid: %d", i);
							exit(1);
						}
						pthread_join(threadid[i], NULL);
					}
				}
		}
	 } else {
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
	}

    close(sockfd);
}