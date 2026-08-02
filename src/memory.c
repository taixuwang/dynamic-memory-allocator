/* memory.c
   374, Memory Homework, 26SP
   Copyright 2026 Taixu Wang
*/

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <assert.h>
#include <pthread.h>
#include "mem.h"  // outward facing functions
#include "mem_internal.h"  // private functions

// Default values for us to use later on
#define NODESIZE sizeof(freeNode)
#define MINCHUNK 16        // smallest allowable chunk of memory
#define BIGCHUNK 16000     // default of a very large chunk size

// Global variables for convenience
// these are static so outside code can't use them.
static freeNode* freeBlockList;  // points to list of available memory blocks
static uintptr_t totalMalloc;   // keeps track of memory allocated with mmap

// Mutex protecting all access to freeBlockList and totalMalloc.
// Internal functions (get_block, new_block, split_node, return_block,
// adjacent, check_heap) are always called while the lock is held.
static pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER;

/* The following functions need to be defined to meet the interface
   specified in mem.h.  These functions return or take the 'usable'
   memory addresses that a user would deal with.  They are called
   in the bench code.
*/

/* getmem returns the address of a usable block of memory that is
   at least size bytes large.  This code calls the helper function
   'get_block'
   Pre-condition: size is a positive integer
*/
void* getmem(uintptr_t size) {
  assert(size > 0);

  // make sure size is a multiple of MINCHUNK (16):
  if (size % MINCHUNK != 0) {
    size = size + MINCHUNK -(size % MINCHUNK);
  }

  pthread_mutex_lock(&heap_lock);
  check_heap();

  uintptr_t block = get_block(size);
  if (block == 0) {
    pthread_mutex_unlock(&heap_lock);
    return NULL;
  }

  check_heap();
  pthread_mutex_unlock(&heap_lock);
  return((void*)(block+NODESIZE));  // offset to get usable address
}

/* freemem uses the functions developed to add blocks to the
   list of available free blocks to return a node to the list.
   The pointer 'p' is the address of usable memory, allocated using getmem
*/
void freemem(void* p) {
  if (p == NULL) {
    return;
  }

  // offset back to get the starting address of the block
  uintptr_t node_address = (uintptr_t)p - NODESIZE;

  pthread_mutex_lock(&heap_lock);
  check_heap();
  return_block(node_address);
  check_heap();
  pthread_mutex_unlock(&heap_lock);
}


uintptr_t get_block(uintptr_t size) {
  freeNode* curr = freeBlockList;
  freeNode* prev = NULL;

  // the extra space must be at least MINCHUNK + NODESIZE to become a block
  while (curr != NULL) {
    if (curr->size >= size + MINCHUNK + NODESIZE) {
      split_node(curr, size);
      if (prev == NULL) {
        freeBlockList = curr->next;
      } else {
        prev->next = curr->next;
      }
      curr->next = NULL;
      return (uintptr_t)curr;
    } else if (curr->size >= size) {
      if (prev == NULL) {
        freeBlockList = curr->next;
      } else {
        prev->next = curr->next;
      }
      curr->next = NULL;
      return (uintptr_t)curr;
    }

    prev = curr;
    curr = curr->next;
  }

  // requests a new block because there is no quilified block in freeBlockList
  freeNode* new_node = new_block(size + NODESIZE);

  if (new_node == NULL) {
    return 0;
  }

  // calls the function again to find the qualified block
  // because new_node may be merged
  return get_block(size);
}

freeNode* new_block(size_t size) {
  size_t alloc_size;
  // allocates the maximum between size and BIGCHUNK
  if (size > BIGCHUNK) {
    alloc_size = size;
  } else {
    alloc_size = BIGCHUNK;
  }

  // Use mmap instead of malloc to get memory directly from the OS.
  // This avoids infinite recursion when this allocator is used as a
  // malloc replacement (via LD_PRELOAD / DYLD_INSERT_LIBRARIES).
  void* raw = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANON, -1, 0);
  if (raw == MAP_FAILED) {
    return NULL;
  }

  freeNode* new_chunk = (freeNode*)raw;
  totalMalloc += alloc_size;

  // gets the usable size
  new_chunk->size = alloc_size - NODESIZE;
  new_chunk->next = NULL;

  return_block((uintptr_t)new_chunk);

  return new_chunk;
}

void split_node(freeNode* n, uintptr_t size) {
  // gets the usable size of the second block
  uintptr_t second_node_size = n->size - NODESIZE - size;
  freeNode* second_node_next = n->next;

  n->size = size;
  // gets the starting adress of the second block
  uintptr_t second_address = (uintptr_t)n + NODESIZE + size;
  freeNode* second_node = (freeNode*)second_address;

  second_node->size = second_node_size;
  second_node->next = second_node_next;

  n->next = second_node;
}


void return_block(uintptr_t node) {
  freeNode* curr_node = (freeNode*)node;

  if (freeBlockList == NULL) {
    freeBlockList = curr_node;
    return;
  }

  if (node < (uintptr_t)freeBlockList) {
    curr_node->next = freeBlockList;
    freeBlockList = curr_node;

    if (adjacent(curr_node)) {
      curr_node->size += curr_node->next->size + NODESIZE;
      curr_node->next = curr_node->next->next;
    }

    return;
  }

  // finds appropriate postion
  freeNode* curr = freeBlockList;
  while (curr->next != NULL && (uintptr_t)curr->next < node) {
    curr = curr->next;
  }

  // inserts the node
  curr_node->next = curr->next;
  curr->next = curr_node;

  // merges blocks if they are adjacent
  if (adjacent(curr_node)) {
    curr_node->size += curr_node->next->size + NODESIZE;
    curr_node->next = curr_node->next->next;
  }
  if (adjacent(curr)) {
    curr->size += curr_node->size + NODESIZE;
    curr->next = curr_node->next;
  }
}

int adjacent(freeNode* node) {
  if (node == NULL || node->next == NULL) {
    return 0;
  }

  if ((uintptr_t)node + NODESIZE + node->size >= (uintptr_t)node->next) {
    return 1;
  }

  return 0;
}

/* The following are utility functions that may prove useful to you.
   They should work as presented, so you can leave them as is.
*/
void check_heap() {
  if (!freeBlockList) return;
  freeNode* currentNode = freeBlockList;
  uintptr_t minsize = currentNode->size;

  while (currentNode != NULL) {
    if (currentNode->size < minsize) {
      minsize = currentNode->size;
    }
    if (currentNode->next != NULL) {
      assert((uintptr_t)currentNode <(uintptr_t)(currentNode->next));
      assert((uintptr_t)currentNode + currentNode->size + NODESIZE
              <(uintptr_t)(currentNode->next));
    }
    currentNode = currentNode->next;
  }
  // go through free list and check for all the things
  if (minsize == 0) print_heap( stdout);
  assert(minsize >= MINCHUNK);
}

void get_mem_stats(uintptr_t* total_size, uintptr_t* total_free,
                   uintptr_t* n_free_blocks) {
  pthread_mutex_lock(&heap_lock);
  *total_size = totalMalloc;
  *total_free = 0;
  *n_free_blocks = 0;

  freeNode* currentNode = freeBlockList;
  while (currentNode) {
    *n_free_blocks = *n_free_blocks + 1;
    *total_free = *total_free + (currentNode->size + NODESIZE);
    currentNode = currentNode->next;
  }
  pthread_mutex_unlock(&heap_lock);
}

void print_heap(FILE *f) {
  pthread_mutex_lock(&heap_lock);
  printf("Printing the heap\n");
  freeNode* currentNode = freeBlockList;
  while (currentNode !=NULL) {
    fprintf(f, "%" PRIuPTR, (uintptr_t)currentNode);
    fprintf(f, ", size: %" PRIuPTR, currentNode->size);
    fprintf(f, ", next: %" PRIuPTR, (uintptr_t)currentNode->next);
    fprintf(f, "\n");
    currentNode = currentNode->next;
  }
  pthread_mutex_unlock(&heap_lock);
}

/* Returns the usable size of a block allocated by getmem.
   The freeNode header is stored immediately before the usable memory,
   so we can read the size field directly.
   Pre-condition: p was returned by getmem and has not been freed.
*/
uintptr_t getmem_usable_size(void* p) {
  if (p == NULL) {
    return 0;
  }
  freeNode* node = (freeNode*)((uintptr_t)p - NODESIZE);
  return node->size;
}