#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_SIZE     4096

// Page table / page directory entry flags (Intel manual vol. 3, ch. 4).
#define PAGE_PRESENT  0x1
#define PAGE_WRITE    0x2
#define PAGE_USER     0x4

// paging_translate() returns this when the address has no mapping.
#define PAGE_UNMAPPED 0xFFFFFFFF

void     paging_init(void);
int      paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
int      paging_unmap(uint32_t virt);
uint32_t paging_translate(uint32_t virt);

int      paging_is_enabled(void);
uint32_t paging_dir_phys(void);
uint32_t paging_identity_end(void);
uint32_t paging_table_count(void);
uint32_t paging_mapped_pages(void);

// Called from the exception handler for interrupt 14.
void     page_fault_handler(uint32_t err_code);

#endif
