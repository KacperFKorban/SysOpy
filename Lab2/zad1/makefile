.DEFAULT_GOAL := compile
CC = gcc -Wall

compile:
	$(CC) -o main main.c

clear:
	rm -f *.o *.a *.so main

test_generate:
	./main generate tmp 100 50

test_sort:
	./main sort tmp 100 50 sys

test:
	./main generate tmp $(SIZE) $(LENGTH)
	cat tmp
	./main sort tmp $(SIZE) $(LENGTH) $(OPT)
	cat tmp
	./main copy tmp tmp1 $(SIZE) $(LENGTH) $(OPT)
	cat tmp1

gen_raport:
	for number in 1 4 512 1024 4096 8192 ; \
	do \
		time -p ./main generate tmp 1000 $$number >> wyniki.txt 2>&1; \
		time -p ./main copy tmp tmp1 1000 $$number sys >> wyniki.txt 2>&1; \
		time -p ./main sort tmp 1000 $$number sys >> wyniki.txt 2>&1; \
		time -p ./main copy tmp1 tmp 1000 $$number lib >> wyniki.txt 2>&1; \
		time -p ./main sort tmp 1000 $$number lib >> wyniki.txt 2>&1; \
		time -p ./main generate tmp 10000 $$number >> wyniki.txt 2>&1; \
		time -p ./main copy tmp tmp1 10000 $$number sys >> wyniki.txt 2>&1; \
		time -p ./main sort tmp 10000 $$number sys >> wyniki.txt 2>&1; \
		time -p ./main copy tmp1 tmp 10000 $$number lib >> wyniki.txt 2>&1; \
		time -p ./main sort tmp 10000 $$number lib >> wyniki.txt 2>&1; \
	done
