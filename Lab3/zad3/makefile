.DEFAULT_GOAL := compile
CC = gcc -Wall

compile:
	$(CC) -o monitor monitor.c
	$(CC) -o tester tester.c

clear:
	rm -f *.o *.a *.so monitor tester

monitor:
	./monitor lista 20 mem

tester:
	./tester ./temp 1 3 16
	