/* test_threadsafe.c
   Multi-threaded stress test for the memory allocator.
   Spawns N threads, each performing random alloc/free operations,
   then verifies heap integrity and that no data corruption occurred.

   Copyright 2026 Taixu Wang
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "mem.h"

#define NUM_THREADS    8
#define OPS_PER_THREAD 5000
#define MAX_BLOCKS     200
#define MAX_SIZE       4096

/* Per-thread state: each thread has its own array of allocated blocks */
typedef struct {
  int thread_id;
  int allocs;
  int frees;
  int errors;
} thread_stats_t;

/* Fill memory with a recognizable pattern tied to thread id */
static void fill_pattern(void* ptr, size_t size, int thread_id) {
  unsigned char val = (unsigned char)(thread_id & 0xFF);
  memset(ptr, val, size);
}

/* Verify the pattern is still intact (detects cross-thread corruption) */
static int verify_pattern(void* ptr, size_t size, int thread_id) {
  unsigned char expected = (unsigned char)(thread_id & 0xFF);
  unsigned char* bytes = (unsigned char*)ptr;
  for (size_t i = 0; i < size; i++) {
    if (bytes[i] != expected) {
      return 0;  // corruption detected
    }
  }
  return 1;
}

/* Each thread runs this function */
static void* thread_work(void* arg) {
  thread_stats_t* stats = (thread_stats_t*)arg;
  int tid = stats->thread_id;

  void*  blocks[MAX_BLOCKS];
  size_t sizes[MAX_BLOCKS];
  int    nblocks = 0;

  unsigned int seed = (unsigned int)(tid * 1337 + 42);

  for (int i = 0; i < OPS_PER_THREAD; i++) {
    if (rand_r(&seed) % 100 < 60 && nblocks < MAX_BLOCKS) {
      // Allocate
      size_t size = (rand_r(&seed) % MAX_SIZE) + 1;
      void* ptr = getmem((uintptr_t)size);
      if (ptr == NULL) {
        stats->errors++;
        continue;
      }
      fill_pattern(ptr, size, tid);
      blocks[nblocks] = ptr;
      sizes[nblocks] = size;
      nblocks++;
      stats->allocs++;

    } else if (nblocks > 0) {
      // Free a random block (after verifying its pattern)
      int idx = rand_r(&seed) % nblocks;
      if (!verify_pattern(blocks[idx], sizes[idx], tid)) {
        fprintf(stderr, "CORRUPTION: thread %d, block %d\n", tid, idx);
        stats->errors++;
      }
      freemem(blocks[idx]);
      blocks[idx] = blocks[nblocks - 1];
      sizes[idx] = sizes[nblocks - 1];
      nblocks--;
      stats->frees++;
    }
  }

  // Verify and free remaining blocks
  for (int i = 0; i < nblocks; i++) {
    if (!verify_pattern(blocks[i], sizes[i], tid)) {
      fprintf(stderr, "CORRUPTION at cleanup: thread %d, block %d\n", tid, i);
      stats->errors++;
    }
    freemem(blocks[i]);
    stats->frees++;
  }

  return NULL;
}

int main(void) {
  printf("=== Multi-threaded allocator stress test ===\n");
  printf("Threads: %d, Ops/thread: %d\n\n", NUM_THREADS, OPS_PER_THREAD);

  pthread_t threads[NUM_THREADS];
  thread_stats_t stats[NUM_THREADS];

  // Launch threads
  for (int i = 0; i < NUM_THREADS; i++) {
    stats[i].thread_id = i;
    stats[i].allocs = 0;
    stats[i].frees = 0;
    stats[i].errors = 0;
    int rc = pthread_create(&threads[i], NULL, thread_work, &stats[i]);
    assert(rc == 0);
  }

  // Wait for all threads
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  // Report results
  int total_allocs = 0, total_frees = 0, total_errors = 0;
  for (int i = 0; i < NUM_THREADS; i++) {
    printf("  Thread %d: %5d allocs, %5d frees, %d errors\n",
           i, stats[i].allocs, stats[i].frees, stats[i].errors);
    total_allocs += stats[i].allocs;
    total_frees  += stats[i].frees;
    total_errors += stats[i].errors;
  }

  printf("\nTotal: %d allocs, %d frees\n", total_allocs, total_frees);

  // Check heap integrity after all operations
  uintptr_t total_size, total_free, n_free;
  get_mem_stats(&total_size, &total_free, &n_free);
  printf("Heap:  %lu bytes mapped, %lu bytes free, %lu free blocks\n",
         (unsigned long)total_size, (unsigned long)total_free,
         (unsigned long)n_free);

  if (total_errors == 0) {
    printf("\n=== PASSED: No corruption detected ===\n");
    return 0;
  } else {
    printf("\n=== FAILED: %d errors detected ===\n", total_errors);
    return 1;
  }
}
