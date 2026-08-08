#include "heap.h"
#include "paging.h"
#include "pmm.h"
#include "vga.h"
#include "util.h"

// A first-fit free list. Every allocation is preceded by a header, and the
// headers form a singly linked list kept in address order:
//
//   [hdr][ payload ][hdr][ payload ][hdr][ free space ...]
//
// Address order is what makes coalescing cheap: two neighbours in the list
// are also neighbours in memory, so merging them is a size adjustment.

#define BLOCK_MAGIC 0x4A415256      // "JARV" - catches double frees
#define MIN_SPLIT   16              // don't split off slivers smaller than this

typedef struct block {
    uint32_t      magic;
    uint32_t      size;             // payload bytes, not counting this header
    struct block* next;
    uint32_t      free;
} block_t;

static block_t* head     = 0;
static uint32_t brk      = 0;       // first unmapped address after the heap
static int      ready    = 0;

static uint32_t align8(uint32_t n) { return (n + 7) & ~7u; }

// Map [brk, brk + bytes) by pulling frames out of the physical allocator.
static int grow_mapping(uint32_t bytes) {
    bytes = (bytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (brk + bytes > HEAP_START + HEAP_MAX) return 0;

    for (uint32_t off = 0; off < bytes; off += PAGE_SIZE) {
        uint32_t frame = pmm_alloc_frame();
        if (frame == 0) return 0;

        if (!paging_map(brk + off, frame, PAGE_WRITE)) {
            pmm_free_frame(frame);
            return 0;
        }
    }
    brk += bytes;
    return 1;
}

// Merge every run of adjacent free blocks into one.
static void coalesce(void) {
    block_t* b = head;
    while (b && b->next) {
        uint8_t* end_of_b = (uint8_t*) b + sizeof(block_t) + b->size;

        if (b->free && b->next->free && end_of_b == (uint8_t*) b->next) {
            b->size += sizeof(block_t) + b->next->size;
            b->next  = b->next->next;
            continue;               // try to swallow the next one as well
        }
        b = b->next;
    }
}

void heap_init(void) {
    brk = HEAP_START;
    if (!grow_mapping(HEAP_INITIAL)) {
        print("FATAL: could not map the kernel heap.\n");
        return;
    }

    head = (block_t*) HEAP_START;
    head->magic = BLOCK_MAGIC;
    head->size  = (brk - HEAP_START) - sizeof(block_t);
    head->next  = 0;
    head->free  = 1;
    ready = 1;
}

// Add newly mapped space to the free list as one more block at the tail.
static int extend_heap(uint32_t payload) {
    uint32_t want = payload + sizeof(block_t);
    if (want < PAGE_SIZE * 4) want = PAGE_SIZE * 4;

    uint32_t old_brk = brk;
    if (!grow_mapping(want)) return 0;

    block_t* fresh = (block_t*) old_brk;
    fresh->magic = BLOCK_MAGIC;
    fresh->size  = (brk - old_brk) - sizeof(block_t);
    fresh->next  = 0;
    fresh->free  = 1;

    block_t* tail = head;
    while (tail->next) tail = tail->next;
    tail->next = fresh;

    coalesce();
    return 1;
}

void* kmalloc(uint32_t size) {
    if (!ready || size == 0) return 0;
    size = align8(size);

    for (int attempt = 0; attempt < 2; attempt++) {
        for (block_t* b = head; b; b = b->next) {
            if (!b->free || b->size < size) continue;

            // Big enough to be worth carving in two? Then split it.
            if (b->size >= size + sizeof(block_t) + MIN_SPLIT) {
                block_t* rest = (block_t*) ((uint8_t*) b + sizeof(block_t) + size);
                rest->magic = BLOCK_MAGIC;
                rest->size  = b->size - size - sizeof(block_t);
                rest->next  = b->next;
                rest->free  = 1;

                b->size = size;
                b->next = rest;
            }

            b->free = 0;
            return (void*) ((uint8_t*) b + sizeof(block_t));
        }

        // Nothing fit: map more memory and look again.
        if (!extend_heap(size)) return 0;
    }
    return 0;
}

int kfree(void* ptr) {
    if (!ptr) return 0;

    // Check the pointer is inside the heap before touching its header --
    // otherwise a stray address sends us reading unmapped memory.
    uint32_t addr = (uint32_t) ptr;
    if (addr < HEAP_START + sizeof(block_t) || addr >= brk) {
        print("kfree: ");
        print_hex(addr);
        print(" is outside the heap\n");
        return 0;
    }

    block_t* b = (block_t*) ((uint8_t*) ptr - sizeof(block_t));
    if (b->magic != BLOCK_MAGIC) {
        print("kfree: bad pointer (no block header) at ");
        print_hex((uint32_t) ptr);
        print("\n");
        return 0;
    }
    if (b->free) {
        print("kfree: double free at ");
        print_hex((uint32_t) ptr);
        print("\n");
        return 0;
    }

    b->free = 1;
    coalesce();
    return 1;
}

uint32_t heap_size(void) { return brk - HEAP_START; }
uint32_t heap_break(void) { return brk; }

uint32_t heap_used(void) {
    uint32_t used = 0;
    for (block_t* b = head; b; b = b->next) {
        used += sizeof(block_t);
        if (!b->free) used += b->size;
    }
    return used;
}

uint32_t heap_free(void) { return heap_size() - heap_used(); }

uint32_t heap_blocks(void) {
    uint32_t n = 0;
    for (block_t* b = head; b; b = b->next) n++;
    return n;
}

uint32_t heap_free_blocks(void) {
    uint32_t n = 0;
    for (block_t* b = head; b; b = b->next) if (b->free) n++;
    return n;
}

void heap_dump(void) {
    print("Heap blocks (address, payload, state):\n");
    int shown = 0;
    for (block_t* b = head; b && shown < 16; b = b->next, shown++) {
        print("  ");
        print_hex((uint32_t) b);
        print("  ");
        print_int(b->size);
        print(b->free ? " bytes  FREE\n" : " bytes  USED\n");
    }
    if (shown == 16) print("  ... (list truncated)\n");
}
