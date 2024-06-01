CC = gcc
FLAGS = -Wall

all:stshell

stshell:stshell.o
	$(CC) $(FLAGS) stshell.o -o stshell -lreadline

stshell.o:stshell.c
	$(CC) $(FLAGS) -c stshell.c


.PHONY: clean all

clean:
	rm -f *.o   stshell
