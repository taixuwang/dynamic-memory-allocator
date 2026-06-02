# Makefile for mem memory system
# muh, CSE 374, 26sp

CC = gcc
CARGS = -Wall -std=c11

all: bench

# basic build
bench: bench.o memory.o
	$(CC) $(CARGS) -o bench $^

# object files
bench.o: bench.c mem.h
	$(CC) $(CARGS) -c bench.c

memory.o: memory.c mem.h
	$(CC) $(CARGS) -c memory.c

## make debug version
debug: CARGS += -g -D DEBUG
debug: bench

noassert: CARGS += -D NDEBUG
noassert: bench

## Utility targetscd
test: debug
	./bench
	./bench 5  # only a few calls
	./bench 10 100 # only getmem	

clean:
	rm -rf bench *.o *~ 
