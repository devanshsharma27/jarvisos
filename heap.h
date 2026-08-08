#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

// The kernel heap lives in virtual memory, well above the identity-mapped
// region, and grows by mapping fresh physical frames as it needs them.
#define HEAP_START   0xC0000000
#define HEAP_INITIAL (64 * 1024)          // mapped up front
#define HEAP_MAX     (4 * 1024 * 1024)    // ceiling on how far it may grow

void  heap_init(void);
void* kmalloc(uint32_t size);
int   kfree(void* ptr);            // 1 if freed, 0 if the pointer was rejected

uint32_t heap_size(void);          // bytes currently mapped
uint32_t heap_used(void);          // bytes handed out (payload + headers)
uint32_t heap_free(void);
uint32_t heap_blocks(void);
uint32_t heap_free_blocks(void);
uint32_t heap_break(void);         // first unmapped address after the heap
void     heap_dump(void);          // print the block list

#endif
