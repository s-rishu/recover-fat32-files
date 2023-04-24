CC=gcc
CFLAGS=-g

.PHONY: all
all: nyufile

nyuenc: nyufile.o

nyuenc.o: nyufile.c

.PHONY: clean
clean:
	rm -f *.o nyufile