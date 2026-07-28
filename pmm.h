#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define FRAME_SIZE 4096

void     pmm_init(void);
uint32_t pmm_alloc_frame(void);        // returns physical address, or 0 if none
void     pmm_free_frame(uint32_t addr);

uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);
uint32_t pmm_bitmap_addr(void);
uint32_t pmm_bitmap_size(void);

#endif
