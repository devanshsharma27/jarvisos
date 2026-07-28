#include "memory.h"
#include "vga.h"

#define MAX_REGIONS 32

static uint32_t total_kb     = 0;
static uint32_t usable_kb    = 0;
static uint32_t highest      = 0;
static int      region_count = 0;

static struct {
    uint32_t base;
    uint32_t len;
    uint32_t type;
} regions[MAX_REGIONS];

void memory_init(multiboot_info_t* mbi) {
    if (!(mbi->flags & (1 << 6))) {
        print("FATAL: GRUB provided no memory map.\n");
        return;
    }

    multiboot_mmap_entry_t* entry =
        (multiboot_mmap_entry_t*) mbi->mmap_addr;
    uint32_t map_end = mbi->mmap_addr + mbi->mmap_length;

    while ((uint32_t) entry < map_end) {
        uint32_t base = (uint32_t) entry->addr;
        uint32_t len  = (uint32_t) entry->len;

        if (region_count < MAX_REGIONS) {
            regions[region_count].base = base;
            regions[region_count].len  = len;
            regions[region_count].type = entry->type;
            region_count++;
        }

        total_kb += len / 1024;

        if (entry->type == 1) {
            usable_kb += len / 1024;
            if (base + len > highest)
                highest = base + len;
        }

        entry = (multiboot_mmap_entry_t*) ((uint32_t) entry + entry->size + 4);
    }
}

void memory_print_map(void) {
    print("Physical memory map (from GRUB):\n");
    for (int i = 0; i < region_count; i++) {
        print("  ");
        print_hex(regions[i].base);
        print(" - ");
        print_hex(regions[i].base + regions[i].len - 1);
        print("  ");
        print_int(regions[i].len / 1024);
        print(" KB  ");
        print(regions[i].type == 1 ? "USABLE\n" : "RESERVED\n");
    }
}

uint32_t mem_total_kb(void)     { return total_kb; }
uint32_t mem_usable_kb(void)    { return usable_kb; }
uint32_t mem_highest_addr(void) { return highest; }
