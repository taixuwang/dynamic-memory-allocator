/* bench.c is the benchmarking /test program for mem memory management
   CSE 374, Memory homework, 24AU
   Copyright 2024 M. Hazen
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "mem.h"

void print_stats(clock_t start);
void fill_mem(void* ptr, uintptr_t size);

/* Synopsis:   bench 
   [ntrials] (10000) getmem + freemem calls
   [pctget] (50) % of calls that are get mem
   [pctlarge] (10) % of calls requesting more memory than lower limit
   [small_limit] (200) largest size in bytes of small block
   [large_limit] (20000) largest size in byes of large block
   [random_seed] (time) initial seed for randn
*/
int main(int argc, char** argv ) {
  // Initialize the parameters
  int NTRIALS;
  int PCTGET;
  int PCTLARGE;
  int SMALL_L;
  int LARGE_L;

  (argc > 1) ? (NTRIALS = atoi(argv[1])) : (NTRIALS = 10000);
  (argc > 2) ? (PCTGET = atoi(argv[2])) : (PCTGET = 50);
  (argc > 3) ? (PCTLARGE = atoi(argv[3])) : (PCTLARGE = 10);
  (argc > 4) ? (SMALL_L = atoi(argv[4])) : (SMALL_L = 200);
  (argc > 5) ? (LARGE_L = atoi(argv[5])) : (LARGE_L = 20000);

  // initialize random number gen.
  (argc > 6) ? srand(atoi(argv[6])) : srand(time(NULL));

  printf("Running bench for %d trials, %d%% getmem calls.\n", NTRIALS, PCTGET);

  void* blocks[NTRIALS];  // upperbound block storage
  int ntrials = 0, nblocks = 0;
  clock_t start;

  // perform NTRIALS mem operations
  start = clock();
  while (ntrials < NTRIALS) {
    #ifdef DEBUG
    printf("ntrial %d\n", ntrials);
    #endif
    if (rand() % 100 < PCTGET) {  // get
      uintptr_t size;  // get random size either small or large
      if (rand() % 100 < PCTLARGE) {
        size =(uintptr_t) (rand() % SMALL_L+1);
      } else {
        size = (uintptr_t) (rand() % (LARGE_L-SMALL_L) + SMALL_L+1);
      }
      #ifdef DEBUG
      printf("get %ld\n", size);
      #endif
      blocks[nblocks] = getmem(size);
      // check for NULL; fill block
      assert(blocks[nblocks] != NULL);
      if (blocks[nblocks] == NULL) {
        fprintf(stderr, "Malloc Failure\n");
        return EXIT_FAILURE;
      }

      fill_mem(blocks[nblocks], size);
      nblocks++;

    } else {  // free
      #ifdef DEBUG
      printf("free %d\n", nblocks);
      #endif
      if (nblocks ==0) {
        ntrials++;
        continue;
      }
      int repblock = rand() % nblocks;
      freemem(blocks[repblock]);
      blocks[repblock] = blocks[nblocks-1];
      nblocks--;
    }
    ntrials++;

    // periodic printing
    if ((NTRIALS > 10) && ntrials % (NTRIALS/10) == 0) {
      print_stats(start);
    }
  }

  assert(ntrials == NTRIALS);
  print_stats(start);
  return EXIT_SUCCESS;
}


void print_stats(clock_t start) {
  uintptr_t storage, free, blocks;
  int msec;
  double aveblock;
  clock_t elapsed = clock() - start;

  msec = (int) (elapsed*1000 / CLOCKS_PER_SEC);
  get_mem_stats(&storage, &free, &blocks);
  (blocks > 0) ? (aveblock = (free / blocks)) : (aveblock = 0);
  printf("Elapsed time: %d msec, Total storage = %d bytes", msec, (int)storage);
  printf("Free blocks = %d, Average block = %f bytes\n", (int)blocks, aveblock);
}

void fill_mem(void* ptr, uintptr_t size) {
  uintptr_t memadd = (uintptr_t) ptr;
  for (int i = 0; i < 16 && i < size; i++) {
    *((unsigned char*)(memadd+i)) = 0xFE;
  }
}

