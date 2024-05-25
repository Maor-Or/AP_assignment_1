CC = gcc
FLAGS = -Wall

all:stshell


copy: copy.o
	$(CC) $(FLAGS) copy.o -o copy

copy.o:copy.c
	$(CC) $(FLAGS) -c copy.c


cmp:cmp.o
	$(CC) $(FLAGS) cmp.o -o cmp

cmp.o:cmp.c
	$(CC) $(FLAGS) -c cmp.c


stshell:stshell.o
	$(CC) $(FLAGS) stshell.o -o stshell

stshell.o:stshell.c
	$(CC) $(FLAGS) -c stshell.c



encode: encode.o
	$(CC) $(FLAGS) encode.o -o encode -ldl

encode.o:encode.c
	$(CC) $(FLAGS) -c encode.c

decode: decode.o
	$(CC) $(FLAGS) decode.o -o decode -ldl

decode.o:decode.c
	$(CC) $(FLAGS) -c decode.c

libcodecA.so:codecA.o
	$(CC) -shared -fPIC codecA.o -o libcodecA.so

libcodecB.so: codecB.o
	$(CC) -shared -fPIC codecB.o -o libcodecB.so

codecA.o:codecA.c codecA.h 
	$(CC) $(FLAGS) -c codecA.c

codecB.o:codecB.c codecB.h 
	$(CC) $(FLAGS) -c codecB.c	


.PHONY: clean all

clean:
	rm -f *.o cmp copy stshell *.so encode decode
