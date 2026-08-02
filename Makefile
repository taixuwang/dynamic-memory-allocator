# Makefile for mymalloc — a drop-in replacement memory allocator
# Copyright 2026 Taixu Wang

# ---- Toolchain ----
CC       = gcc
CFLAGS   = -Wall -std=c11 -pthread -I$(INC_DIR)
AR       = ar
ARFLAGS  = rcs

# ---- Directories ----
SRC_DIR   = src
INC_DIR   = include
BENCH_DIR = bench
TESTS_DIR = tests
BUILD_DIR = build

# ---- OS detection ----
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    SHARED_EXT   = dylib
    SHARED_FLAGS = -dynamiclib
else
    SHARED_EXT   = so
    SHARED_FLAGS = -shared -Wl,-soname,$(LIB_NAME).$(SHARED_EXT)
endif

# ---- Naming ----
LIB_NAME  = libmymalloc
LIB_SO    = $(BUILD_DIR)/$(LIB_NAME).$(SHARED_EXT)
LIB_A     = $(BUILD_DIR)/$(LIB_NAME).a
BENCH_BIN = $(BUILD_DIR)/bench

# ---- Install paths ----
PREFIX      ?= /usr/local
INSTALL_LIB  = $(PREFIX)/lib
INSTALL_INC  = $(PREFIX)/include/mymalloc

# ---- Source / object lists ----
LIB_SRCS     = $(SRC_DIR)/memory.c $(SRC_DIR)/malloc_wrapper.c
LIB_OBJS     = $(BUILD_DIR)/memory.o $(BUILD_DIR)/malloc_wrapper.o
LIB_PIC_OBJS = $(BUILD_DIR)/memory_pic.o $(BUILD_DIR)/malloc_wrapper_pic.o
BENCH_OBJS   = $(BUILD_DIR)/bench.o $(BUILD_DIR)/memory.o

# ==============================================================
#  Targets
# ==============================================================

.PHONY: all lib clean test debug noassert install uninstall

all: $(BENCH_BIN) $(LIB_SO) $(LIB_A)

lib: $(LIB_SO) $(LIB_A)

# ---- Build directory (order-only prerequisite) ----
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ---- Shared library ----
$(LIB_SO): $(LIB_PIC_OBJS)
	$(CC) $(CFLAGS) $(SHARED_FLAGS) -o $@ $^

# ---- Static library ----
$(LIB_A): $(LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

# ---- Benchmark executable (links getmem/freemem directly) ----
$(BENCH_BIN): $(BENCH_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# ---- PIC objects (for shared library) ----
$(BUILD_DIR)/memory_pic.o: $(SRC_DIR)/memory.c $(INC_DIR)/mem.h $(INC_DIR)/mem_internal.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(BUILD_DIR)/malloc_wrapper_pic.o: $(SRC_DIR)/malloc_wrapper.c $(INC_DIR)/mem.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

# ---- Regular objects ----
$(BUILD_DIR)/memory.o: $(SRC_DIR)/memory.c $(INC_DIR)/mem.h $(INC_DIR)/mem_internal.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/malloc_wrapper.o: $(SRC_DIR)/malloc_wrapper.c $(INC_DIR)/mem.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bench.o: $(BENCH_DIR)/bench.c $(INC_DIR)/mem.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Debug / release variants ----
debug: CFLAGS += -g -DDEBUG
debug: $(BENCH_BIN)

noassert: CFLAGS += -DNDEBUG
noassert: $(BENCH_BIN)

# ---- Test ----
TEST_THREAD = $(BUILD_DIR)/test_threadsafe

$(TEST_THREAD): $(TESTS_DIR)/test_threadsafe.c $(BUILD_DIR)/memory.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(BUILD_DIR)/memory.o

test: debug $(TEST_THREAD)
	$(BENCH_BIN)
	$(BENCH_BIN) 5
	$(BENCH_BIN) 10 100
	$(TEST_THREAD)

# ---- Install / Uninstall ----
install: $(LIB_SO) $(LIB_A)
	install -d $(INSTALL_LIB) $(INSTALL_INC)
	install -m 755 $(LIB_SO) $(INSTALL_LIB)/
	install -m 644 $(LIB_A)  $(INSTALL_LIB)/
	install -m 644 $(INC_DIR)/mem.h $(INSTALL_INC)/

uninstall:
	rm -f  $(INSTALL_LIB)/$(LIB_NAME).$(SHARED_EXT)
	rm -f  $(INSTALL_LIB)/$(LIB_NAME).a
	rm -rf $(INSTALL_INC)

# ---- Clean ----
clean:
	rm -rf $(BUILD_DIR)
