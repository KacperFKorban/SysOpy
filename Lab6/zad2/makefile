.DEFAULT_GOAL := compile
CC = gcc -Wall

compile:
	$(CC) -o server server.c -lrt
	$(CC) -o client client.c -lrt

clear:
	rm -f *.o *.a *.so server client