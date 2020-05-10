#Ameer Khan
#CMPE 156
#Final Project
#makefile

all: client server

client: client.c 
	gcc -lpthread -o client client.c -lrt -g
	mv client ../bin/

server: server.c 
	gcc -lpthread -o server server.c -lrt
	mv server ../bin/

clean: 
	rm ../bin/client
	rm ../bin/server