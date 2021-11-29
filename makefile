all: cjnsh
	
cjnsh: main.o help.o
	gcc main.o help.o -o cjnsh

main.o: main.c main.h
	gcc -c main.c
	
help.o: help.c help.h 
	gcc -c help.c

run:
	./cjnsh
