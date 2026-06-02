/* mem.h is the public header for memory functions 
   CSE 374, Memory homework, 26SP
   Copyright 2026 Taixu Wang
*/

#ifndef MEM_H_
#define MEM_H_

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

/* Return a pointer to a new block of storage with at least 'size' bytes space. 
   The 'size' value must be positive.  If it is not, or if there is another
   problem allocating the memory the function should return NULL. 
*/
void* getmem(uintptr_t size);

/* Return the block of storage at location p to the pool of available free 
   storage. The pointer value p must be one that was obtained as the result 
   of a call to getmem. If p is NULL, then the call to freemem has no effect 
   and returns immediately. If p has not been allocated using getmem the
   behavior of the function is undefined.
*/
void freemem(void* p);

/* Store statistics about the current state of the memory manager in the 
   three integer variables whose addresses are given as arguments. The 
   information stored should be as follows:
   total_size: total amount of storage in bytes acquired by the memory manager
   total_free: the total amount of storage in bytes that is currently stored 
   on the free list.
   n_free_blocks: the total number of individual blocks currently stored 
   on the free list. */
void get_mem_stats(uintptr_t* total_size, uintptr_t* total_free,
                   uintptr_t* n_free_blocks);


#endif  // MEM_H_