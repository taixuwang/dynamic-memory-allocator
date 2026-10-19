/* benchmark.c
   Performance comparison: mymalloc vs system malloc (glibc/macOS libsystem).

   Runs identical workloads through both allocators and outputs CSV data
   for visualization with the companion plot_benchmark.py script.

   Compile (done automatically by Makefile):
     gcc -Wall -std=c11 -pthread -Iinclude -o build/benchmark \
         bench/benchmark.c src/memory.c

   Copyright 2026 Taixu Wang
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include "mem.h"

/* ---- Configuration ---- */
#define SEED           42
#define WARMUP_OPS     1000
#define MAX_BLOCKS     50000

/* Workload scenarios */
typedef struct {
  const char* name;
  int   ntrials;      /* total alloc+free operations */
  int   pct_alloc;    /* percentage of ops that are alloc (vs free) */
  int   small_max;    /* max size for "small" allocations */
  int   large_max;    /* max size for "large" allocations */
  int   pct_small;    /* percentage of allocs that are "small" */
} workload_t;

static workload_t workloads[] = {
  {"small_heavy",   100000, 50, 64,    512,   90},
  {"mixed",         100000, 50, 256,   8192,  50},
  {"large_heavy",   100000, 50, 1024,  65536, 10},
  {"alloc_burst",   100000, 80, 256,   4096,  50},
  {"free_heavy",    100000, 30, 256,   4096,  50},
};
#define NUM_WORKLOADS (sizeof(workloads) / sizeof(workloads[0]))

/* ---- Timing helpers ---- */

/* Returns wall-clock time in nanoseconds */
static long long now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/* ---- Workload runner ---- */

typedef struct {
  long long elapsed_ns;
  int       total_allocs;
  int       total_frees;
  long long peak_held;    /* peak bytes held at once (approx) */
} bench_result_t;

/*
   Run a workload using function pointers for alloc/free.
   This allows us to run the EXACT same random sequence with
   both our allocator and the system allocator.
*/
static bench_result_t run_workload(
    workload_t* w,
    void* (*alloc_fn)(size_t),
    void  (*free_fn)(void*))
{
  bench_result_t result = {0};

  void**  blocks = (void**)calloc(MAX_BLOCKS, sizeof(void*));
  size_t* sizes  = (size_t*)calloc(MAX_BLOCKS, sizeof(size_t));
  assert(blocks && sizes);

  int nblocks = 0;
  long long held = 0;
  long long peak = 0;

  /* Use a deterministic seed so both allocators see the same sequence */
  unsigned int seed = SEED;

  /* Warmup (not timed) */
  for (int i = 0; i < WARMUP_OPS && i < w->ntrials / 10; i++) {
    size_t sz = (rand_r(&seed) % w->small_max) + 1;
    void* p = alloc_fn(sz);
    if (p) free_fn(p);
  }

  /* Reset seed for the real run */
  seed = SEED;

  long long t_start = now_ns();

  for (int i = 0; i < w->ntrials; i++) {
    if (rand_r(&seed) % 100 < w->pct_alloc && nblocks < MAX_BLOCKS) {
      /* Allocate */
      size_t sz;
      if (rand_r(&seed) % 100 < w->pct_small) {
        sz = (rand_r(&seed) % w->small_max) + 1;
      } else {
        sz = (rand_r(&seed) % (w->large_max - w->small_max)) + w->small_max + 1;
      }

      void* ptr = alloc_fn(sz);
      if (ptr == NULL) continue;

      /* Touch memory to make it real */
      memset(ptr, 0xAB, sz < 64 ? sz : 64);

      blocks[nblocks] = ptr;
      sizes[nblocks]  = sz;
      nblocks++;
      result.total_allocs++;
      held += (long long)sz;
      if (held > peak) peak = held;

    } else if (nblocks > 0) {
      /* Free a random block */
      int idx = rand_r(&seed) % nblocks;
      free_fn(blocks[idx]);
      held -= (long long)sizes[idx];
      blocks[idx] = blocks[nblocks - 1];
      sizes[idx]  = sizes[nblocks - 1];
      nblocks--;
      result.total_frees++;
    }
  }

  long long t_end = now_ns();

  /* Cleanup remaining blocks */
  for (int i = 0; i < nblocks; i++) {
    free_fn(blocks[i]);
  }

  result.elapsed_ns = t_end - t_start;
  result.peak_held  = peak;

  /* Use system free for our tracking arrays */
  free(blocks);
  free(sizes);

  return result;
}

/* ---- Wrappers for mymalloc (via getmem/freemem) ---- */

static void* mymalloc_alloc(size_t size) {
  return getmem((uintptr_t)size);
}

static void mymalloc_free(void* ptr) {
  freemem(ptr);
}

/* ---- Main ---- */

int main(int argc, char** argv) {
  const char* csv_file = "benchmark_results.csv";
  if (argc > 1) {
    csv_file = argv[1];
  }

  FILE* fp = fopen(csv_file, "w");
  if (!fp) {
    fprintf(stderr, "Error: cannot open %s for writing\n", csv_file);
    return 1;
  }

  /* CSV header */
  fprintf(fp, "workload,allocator,ops,allocs,frees,elapsed_us,throughput_mops,peak_held_kb\n");

  printf("╔══════════════════════════════════════════════════════════════════╗\n");
  printf("║         Memory Allocator Performance Benchmark                 ║\n");
  printf("╚══════════════════════════════════════════════════════════════════╝\n\n");

  for (int w = 0; w < (int)NUM_WORKLOADS; w++) {
    workload_t* wl = &workloads[w];
    printf("── Workload: %-15s (%d ops, %d%% alloc, %d%% small) ──\n",
           wl->name, wl->ntrials, wl->pct_alloc, wl->pct_small);

    /* Run system malloc */
    bench_result_t sys_result = run_workload(wl, malloc, free);
    double sys_us  = sys_result.elapsed_ns / 1000.0;
    double sys_mops = (double)wl->ntrials / (sys_result.elapsed_ns / 1000.0);

    printf("  system malloc:  %8.0f µs  (%6.2f M ops/sec)  peak=%lld KB\n",
           sys_us, sys_mops, sys_result.peak_held / 1024);

    fprintf(fp, "%s,system,%d,%d,%d,%.0f,%.4f,%lld\n",
            wl->name, wl->ntrials, sys_result.total_allocs, sys_result.total_frees,
            sys_us, sys_mops, sys_result.peak_held / 1024);

    /* Run mymalloc */
    bench_result_t my_result = run_workload(wl, mymalloc_alloc, mymalloc_free);
    double my_us  = my_result.elapsed_ns / 1000.0;
    double my_mops = (double)wl->ntrials / (my_result.elapsed_ns / 1000.0);

    printf("  mymalloc:       %8.0f µs  (%6.2f M ops/sec)  peak=%lld KB\n",
           my_us, my_mops, my_result.peak_held / 1024);

    double ratio = sys_us / my_us;
    printf("  ratio:          mymalloc is %.2fx %s\n\n",
           ratio > 1.0 ? ratio : 1.0 / ratio,
           ratio > 1.0 ? "faster" : "slower");

    fprintf(fp, "%s,mymalloc,%d,%d,%d,%.0f,%.4f,%lld\n",
            wl->name, wl->ntrials, my_result.total_allocs, my_result.total_frees,
            my_us, my_mops, my_result.peak_held / 1024);
  }

  fclose(fp);
  printf("Results written to: %s\n", csv_file);
  printf("Run `python3 scripts/plot_benchmark.py %s` to generate charts.\n", csv_file);

  return 0;
}
