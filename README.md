# mymalloc — A Dynamic Memory Allocator

A user-space dynamic memory allocator written in C that pre-allocates large
memory chunks via `mmap`, then serves subsequent allocations entirely in
user-space to reduce expensive system calls and kernel context switches.

It can be used as a **drop-in replacement** for the system `malloc` via
`LD_PRELOAD` (Linux) or `DYLD_INSERT_LIBRARIES` (macOS).

## Features

- **Explicit free-list** with address-ordered insertion and automatic coalescing
  of adjacent blocks to reduce fragmentation.
- **Block splitting** — oversized free blocks are split so the surplus stays on
  the free list.
- **16-byte alignment** — every returned pointer is aligned to a 16-byte
  boundary.
- **Thread-safe** — a `pthread_mutex` guards all free-list operations, verified
  by a 32-thread stress test.
- **Heap integrity checks** — `check_heap()` runs assertions after every
  `getmem` / `freemem` call (disabled with `-DNDEBUG`).
- **POSIX `malloc` wrapper** — `malloc`, `free`, `calloc`, and `realloc` are
  implemented on top of the internal `getmem` / `freemem` API, allowing the
  library to be injected into unmodified programs.
- **Benchmark suite** — compare mymalloc against the system allocator across
  five workload profiles, with CSV output and chart generation.

## Project Structure

```
├── include/
│   ├── mem.h              # Public API (getmem, freemem, get_mem_stats)
│   └── mem_internal.h     # Internal declarations (free-list helpers)
├── src/
│   ├── memory.c           # Core allocator implementation
│   └── malloc_wrapper.c   # POSIX malloc/free/calloc/realloc wrappers
├── bench/
│   ├── bench.c            # Functional test / stress bench
│   └── benchmark.c        # Performance comparison vs system malloc
├── tests/
│   └── test_threadsafe.c  # Multi-threaded correctness test (32 threads)
├── scripts/
│   └── plot_benchmark.py  # Generate charts from benchmark CSV
└── Makefile               # Build system (static lib, shared lib, tests, bench)
```

## Requirements

- **GCC** (or any C11-compatible compiler)
- **POSIX** environment (Linux or macOS)
- **Python 3** + **matplotlib** (optional, for benchmark charts)

## Building

```bash
# Build everything (benchmark binary + static & shared libraries)
make

# Build only the libraries
make lib

# Debug build (includes -g and -DDEBUG)
make debug

# Release build (disables assert via -DNDEBUG)
make noassert

# Clean all build artifacts
make clean
```

### Build Outputs

| File | Description |
|------|-------------|
| `build/libmymalloc.a` | Static library |
| `build/libmymalloc.dylib` | Shared library (macOS) |
| `build/libmymalloc.so` | Shared library (Linux) |
| `build/bench` | Functional test / stress program |

## API

### Public Interface (`include/mem.h`)

```c
void*     getmem(uintptr_t size);
void      freemem(void* p);
void      get_mem_stats(uintptr_t* total_size, uintptr_t* total_free,
                        uintptr_t* n_free_blocks);
uintptr_t getmem_usable_size(void* p);
```

### POSIX Wrappers (`src/malloc_wrapper.c`)

Standard `malloc`, `free`, `calloc`, and `realloc` — built into the shared
library so it can replace the system allocator at load time.

## Usage

### As a Library

Link against `libmymalloc.a` and use `getmem` / `freemem` directly:

```c
#include "mem.h"

void* p = getmem(1024);
// ...
freemem(p);
```

### As a Drop-in malloc Replacement

**Linux:**
```bash
LD_PRELOAD=./build/libmymalloc.so ./your_program
```

**macOS:**
```bash
DYLD_INSERT_LIBRARIES=./build/libmymalloc.dylib \
  DYLD_FORCE_FLAT_NAMESPACE=1 \
  ./your_program
```

## Testing

```bash
# Run functional tests + thread-safety test
make test
```

The test suite includes:
- **Functional bench** — randomized alloc/free sequences with heap
  integrity validation.
- **Thread-safety test** — 32 threads × 100,000 operations each, with
  per-thread memory patterns to detect cross-thread corruption.

## Benchmarking

```bash
# Run performance benchmark (mymalloc vs system malloc)
make benchmark

# Generate charts from benchmark results
python3 scripts/plot_benchmark.py build/benchmark_results.csv
```

The benchmark runs five workload scenarios:

| Workload | Description |
|----------|-------------|
| `small_heavy` | 90% small allocations (≤ 64 B) |
| `mixed` | 50/50 small/large allocations |
| `large_heavy` | 90% large allocations (up to 64 KB) |
| `alloc_burst` | 80% allocations, 20% frees |
| `free_heavy` | 30% allocations, 70% frees |

## Installation

```bash
# Install to /usr/local (or set PREFIX)
make install

# Uninstall
make uninstall
```

## License

Copyright 2026 Taixu Wang