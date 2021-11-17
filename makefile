all: cjnsh
	
cjnsh: main.o
	gcc main.o -o cjnsh

main.o: main.c
	gcc -c main.c

run:
	./cjnsh
