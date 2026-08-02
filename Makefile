# Makefile for mem memory system
# Copyright 2026 Taixu Wang

CC = gcc
CARGS = -Wall -std=c11

# Detect OS for shared library settings
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    SHARED_EXT = dylib
    SHARED_FLAGS = -dynamiclib
    PRELOAD_CMD = DYLD_INSERT_LIBRARIES=./libmymalloc.dylib DYLD_FORCE_FLAT_NAMESPACE=1
else
    SHARED_EXT = so
    SHARED_FLAGS = -shared
    PRELOAD_CMD = LD_PRELOAD=./libmymalloc.so
endif

LIB_NAME = libmymalloc

# ---- Default target ----
all: bench $(LIB_NAME).$(SHARED_EXT) $(LIB_NAME).a

# ---- Shared library (core deliverable) ----
$(LIB_NAME).$(SHARED_EXT): memory_pic.o malloc_wrapper_pic.o
	$(CC) $(CARGS) $(SHARED_FLAGS) -o $@ $^

# PIC object files for shared library
memory_pic.o: memory.c mem.h mem_internal.h
	$(CC) $(CARGS) -fPIC -c memory.c -o $@

malloc_wrapper_pic.o: malloc_wrapper.c mem.h
	$(CC) $(CARGS) -fPIC -c malloc_wrapper.c -o $@

# ---- Static library ----
$(LIB_NAME).a: memory.o malloc_wrapper.o
	ar rcs $@ $^

malloc_wrapper.o: malloc_wrapper.c mem.h
	$(CC) $(CARGS) -c malloc_wrapper.c

# ---- Benchmark executable (uses getmem/freemem directly) ----
bench: bench.o memory.o
	$(CC) $(CARGS) -o bench $^

bench.o: bench.c mem.h
	$(CC) $(CARGS) -c bench.c

memory.o: memory.c mem.h mem_internal.h
	$(CC) $(CARGS) -c memory.c

# ---- Debug / noassert builds ----
debug: CARGS += -g -D DEBUG
debug: bench

noassert: CARGS += -D NDEBUG
noassert: bench

# ---- Test targets ----
test: debug
	./bench
	./bench 5
	./bench 10 100

# ---- Clean ----
clean:
	rm -rf bench *.o *.$(SHARED_EXT) *.a *~

.PHONY: all clean test debug noassert
