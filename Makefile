CC = gcc
FLAGS = -Wall

all:myshell

myshell:stshell.o
	$(CC) $(FLAGS) stshell.o -o myshell -lreadline

stshell.o:stshell.c
	$(CC) $(FLAGS) -c stshell.c


.PHONY: clean all

clean:
	rm -f *.o   myshell
