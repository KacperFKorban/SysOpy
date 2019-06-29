.DEFAULT_GOAL := compile
CC = gcc -Wall

compile:
	$(CC) -o client client.c -lm -pthread
	$(CC) -o server server.c -lm -pthread

clear:
	rm -f *.o *.a *.so client server