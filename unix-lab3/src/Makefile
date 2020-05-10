#Ameer Khan
#CMPE 156
#Assignment #3
#makefile

all: client server

client: client.c 
	gcc -lpthread -o client client.c
	mv client ../bin/

server: server.c 
	gcc -lpthread -o server server.c
	mv server ../bin/

clean: 
	rm ../bin/client
	rm ../bin/server
