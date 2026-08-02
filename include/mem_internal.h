/* mem_internal.h
   374, Memory Homework, 26SP
   Internal declarations for memory system
   Copyright 2026 Taixu Wang
*/

#ifndef MEM_INTERNAL_H_
#define MEM_INTERNAL_H_

// Necessary libraries
#include <inttypes.h>
#include <stdio.h>

// Define the major data structure that is used to track blocks of memory
// freeNode will hold the info about the block + pointer to the next node
typedef struct freeNode {
  uintptr_t size;  // size of usable memory
  struct freeNode *next;
} freeNode;

/* Retrieve one memory block from the free list.
   size specifies the number of usable bytes required
   Assumes size is an even multiple of MINCHUNK
       as required for memory alignment
   Will also update the free list IF necessary:
       If there is no sufficiently large block of memory in the list,
           requests a new large block using malloc (calls new_block)
       If the block identified islarger than required,
           splits blocks into smaller blocks if necessary (call split_block)
   returns address of the start of the freeNode struct
   Pre-condition: size is a positive integer
   Pre-condition: An empty or sorted list of available memory blocks
   Post-condition: An empty or sorted list of available memory blocks
*/
uintptr_t get_block(uintptr_t size);

/* creates a new block by mallocing a big chunk of memory
   size is required bytes: allocates max (big chunk / size)
   inserts the new block into the freeBlockList in the correct place
   (call return_block)
   Pre-condition: size is a positive integer
   Pre-condition: an empty or sorted list of memory blocks
   Post-condition: a sorted list of memory blocks
*/
freeNode* new_block(size_t size);

/* splits one node into two, the first of which has size == size
   the second of which is the remaining space in the node.
   Pre-condition: size is a positive integer
   Pre-condition: n has node_size > size + MINCHUNK + NODESIZE
   Post-condition: both nodes remain on the freeBlockList 
   Post-condition: the freeBlockList is sorted and non-empty
*/
void split_node(freeNode* n, uintptr_t size);

/* inserts a block back into the free list in the right place 
   will also merge the block to it neighbors if necessary 
   if free list is empty, assigns free list = node
   Pre-condition: an empty or sorted list of available memory blocks
   Pre-condition: The list satisfies all checks in check_heap
   Post-condition: a sorted list of memory blocks
   Post-condition: the list satisfies all checks in check_heap
*/
void return_block(uintptr_t node);

/* returns 1 if node is adjacent to the subsequent one
   adjacent implies that the 'end' of the first block has an address
   greater than or equal to the address of the second block
*/
int adjacent(freeNode* node);


/* Check for possible problems with the free list data structure. 
   When called this function should use asserts to verify that the free 
   list has the following properties:
   Blocks are ordered with strictly increasing memory addresses
   Block sizes are positive numbers and large than the minimum
   Blocks do not overlap 
   Blocks are not touching 
   If no errors are detected, this function should return silently. 
*/
void check_heap();

/* Print a formatted listing on file f showing the blocks on the free list. */
void print_heap(FILE * f);

#endif  // MEM_INTERNAL_H_