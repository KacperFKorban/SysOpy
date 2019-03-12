CC = gcc -Wall

all: static shared clear

static:
	make clear
	$(CC) -c library.c
	ar rcs library.a library.o
	$(CC) -o main main.c library.a

shared:
	make clear
	$(CC) -c -fPIC library.c
	$(CC) -shared -fPIC -o library.so library.o
	$(CC) -o main main.c -L. library.so -Wl,-rpath=`pwd`

dynamic:
	make clear
	$(CC) -c -fPIC library.c
	$(CC) -shared -fPIC -o library.so library.o
	$(CC) -o main main_dynamic.c -ldl

clear:
	rm -f *.o *.a *.so main

small:
	./main create_table 10 search_directory /usr/lib . tmp

medium:
	./main create_table 1000 search_directory /usr . tmp

big:
	./main create_table 1000000 search_directory / . tmp

create_delete:
	./main create_table 1000 search_directory /usr . tmp remove_block 0 search_directory /usr . tmp remove_block 1 search_directory /usr . tmp remove_block 2 search_directory /usr . tmp remove_block 3 search_directory /usr . tmp remove_block 4

all_tests:
	make small
	make medium
	make big
	make create_delete
