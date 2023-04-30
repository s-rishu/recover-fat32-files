CC=gcc
CFLAGS=-g
LDFLAGS=-lcrypto

.PHONY: all
all: nyufile

nyuenc: nyufile.o

nyuenc.o: nyufile.c

.PHONY: clean
clean:
	rm -f *.o nyufile