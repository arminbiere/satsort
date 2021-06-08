CC=gcc
CFLAGS=-Wall -g

all: satsort

satsort: satsort.o ../kissat/build/libkissat.a makefile
	$(CC) $(CFLAGS) -o $@ $< -L../kissat/build -lkissat
satsort.o: satsort.c ../kissat/src/kissat.h makefile
	$(CC) $(CFLAGS) -c -o $@ -I../kissat/src $<

indent:
	indent satsort.c
clean:
	rm -f satsort *.o *~
