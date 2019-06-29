CC = gcc -Wall -std=c11 -ggdb


all: static shared quit

static:
	$(CC) -c library.c
	ar rcs library.a library.o

shared:
	$(CC) -c -fPIC library.c
	$(CC) -shared -fPIC -o library.so library.o

clear:
	rm -f *.o *.a *.so main