.DEFAULT_GOAL := compile
CC = gcc -Wall

compile:
	$(CC) -o trucker trucker.c -lpthread -lrt
	$(CC) -o loader loader.c -lpthread -lrt

clear:
	rm -f *.o *.a *.so trucker loader