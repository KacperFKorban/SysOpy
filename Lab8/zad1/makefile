.DEFAULT_GOAL := compile
CC = gcc -Wall

compile:
	$(CC) -o main main.c -lm -pthread

test:
	for number in 1 2 4 8 ; do \
		./main $$number block baboon.ascii.pgm filter31 out.ascii.pgm; \
	done

clear:
	rm -f *.o *.a *.so main out.ascii.pgm