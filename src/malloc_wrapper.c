/* malloc_wrapper.c
   POSIX-compatible malloc/free/calloc/realloc interface.

   Wraps the internal getmem/freemem allocator so it can serve as a
   drop-in replacement for the system malloc.

   Usage (Linux):
     LD_PRELOAD=./libmymalloc.so ./your_program
   Usage (macOS):
     DYLD_INSERT_LIBRARIES=./libmymalloc.dylib DYLD_FORCE_FLAT_NAMESPACE=1 \
       ./your_program

   Copyright 2026 Taixu Wang
*/

#include <stddef.h>
#include <string.h>
#include "mem.h"

/* ---- Standard POSIX memory allocation interface ---- */

/* Allocate 'size' bytes of uninitialized memory.
   Returns NULL if size is 0 or allocation fails. */
void* malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  return getmem((uintptr_t)size);
}

/* Release the memory block at 'ptr'.
   If ptr is NULL, no operation is performed. */
void free(void* ptr) {
  freemem(ptr);
}

/* Allocate memory for an array of 'count' elements of 'size' bytes each.
   The memory is zero-initialized.
   Returns NULL on overflow, zero-size, or allocation failure. */
void* calloc(size_t count, size_t size) {
  if (count == 0 || size == 0) {
    return NULL;
  }

  // Overflow check: count * size must not wrap around
  size_t total = count * size;
  if (total / count != size) {
    return NULL;
  }

  void* ptr = getmem((uintptr_t)total);
  if (ptr != NULL) {
    memset(ptr, 0, total);
  }
  return ptr;
}

/* Resize the memory block at 'ptr' to 'size' bytes.
   - If ptr is NULL, equivalent to malloc(size).
   - If size is 0, frees ptr and returns NULL.
   - Otherwise allocates a new block, copies existing data, and frees the old.
   Returns NULL if allocation fails (old block is left unchanged). */
void* realloc(void* ptr, size_t size) {
  if (ptr == NULL) {
    return malloc(size);
  }
  if (size == 0) {
    free(ptr);
    return NULL;
  }

  uintptr_t old_size = getmem_usable_size(ptr);

  void* new_ptr = malloc(size);
  if (new_ptr == NULL) {
    return NULL;  // original block is NOT freed
  }

  // Copy the minimum of old and new sizes
  size_t copy_size = (old_size < size) ? old_size : size;
  memcpy(new_ptr, ptr, copy_size);

  free(ptr);
  return new_ptr;
}
