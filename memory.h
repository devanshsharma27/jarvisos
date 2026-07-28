#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "multiboot.h"

void memory_init(multiboot_info_t* mbi);
void memory_print_map(void);
uint32_t mem_total_kb(void);
uint32_t mem_usable_kb(void);
uint32_t mem_highest_addr(void);

int      mem_region_count(void);
uint32_t mem_region_base(int i);
uint32_t mem_region_len(int i);
uint32_t mem_region_type(int i);

#endif
