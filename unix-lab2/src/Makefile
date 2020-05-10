#Ameer Khan
#CMPE 156
#Assignment #2
#makefile

all: client server

client: client.c 
	gcc -o client client.c
	mv client ../bin

server: server.c 
	gcc -o server server.c
	mv server ../bin

clean: 
	rm ../bin/client
	rm ../bin/server