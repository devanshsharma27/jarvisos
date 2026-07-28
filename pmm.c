#include "pmm.h"
#include "memory.h"
#include "vga.h"

// Provided by the linker script: address of the end of the kernel.
extern uint32_t kernel_end;

static uint32_t* bitmap       = 0;   // one bit per frame
static uint32_t  total_frames = 0;
static uint32_t  used_frames  = 0;
static uint32_t  bitmap_size  = 0;   // in bytes

// ---- bit helpers -------------------------------------------------
// Frame N lives in bitmap word N/32, at bit position N%32.

static int frame_test(uint32_t frame) {
    return (bitmap[frame / 32] >> (frame % 32)) & 1;
}

static void frame_mark_used(uint32_t frame) {
    if (frame >= total_frames) return;
    if (!frame_test(frame)) {
        bitmap[frame / 32] |= (1u << (frame % 32));
        used_frames++;
    }
}

static void frame_mark_free(uint32_t frame) {
    if (frame >= total_frames) return;
    if (frame_test(frame)) {
        bitmap[frame / 32] &= ~(1u << (frame % 32));
        used_frames--;
    }
}

// ---- initialisation ----------------------------------------------

void pmm_init(void) {
    uint32_t highest = mem_highest_addr();
    total_frames = highest / FRAME_SIZE;

    // Bytes needed: one bit per frame, rounded up to whole 4-byte words.
    bitmap_size = ((total_frames + 31) / 32) * 4;

    // Park the bitmap immediately after the kernel, 4KB-aligned.
    uint32_t bitmap_addr = (uint32_t) &kernel_end;
    bitmap_addr = (bitmap_addr + FRAME_SIZE - 1) & ~(FRAME_SIZE - 1);
    bitmap = (uint32_t*) bitmap_addr;

    // Start pessimistic: mark EVERYTHING as used.
    for (uint32_t i = 0; i < bitmap_size / 4; i++)
        bitmap[i] = 0xFFFFFFFF;
    used_frames = total_frames;

    // Now free only the regions GRUB told us are usable RAM (type 1).
    for (int r = 0; r < mem_region_count(); r++) {
        if (mem_region_type(r) != 1) continue;

        uint32_t base = mem_region_base(r);
        uint32_t len  = mem_region_len(r);
        uint32_t start_frame = base / FRAME_SIZE;
        uint32_t frame_count = len / FRAME_SIZE;

        for (uint32_t f = 0; f < frame_count; f++)
            frame_mark_free(start_frame + f);
    }

    // Re-claim everything from address 0 through the end of the bitmap:
    // that covers low memory, the kernel image, and the bitmap itself.
    uint32_t reserved_end = bitmap_addr + bitmap_size;
    uint32_t last_reserved_frame = reserved_end / FRAME_SIZE;
    for (uint32_t f = 0; f <= last_reserved_frame; f++)
        frame_mark_used(f);
}

// ---- allocation --------------------------------------------------

uint32_t pmm_alloc_frame(void) {
    // Scan word-by-word; skip fully-used words instantly.
    for (uint32_t w = 0; w < bitmap_size / 4; w++) {
        if (bitmap[w] == 0xFFFFFFFF) continue;

        for (uint32_t b = 0; b < 32; b++) {
            if (!((bitmap[w] >> b) & 1)) {
                uint32_t frame = w * 32 + b;
                if (frame >= total_frames) return 0;
                frame_mark_used(frame);
                return frame * FRAME_SIZE;
            }
        }
    }
    return 0;   // out of memory
}

void pmm_free_frame(uint32_t addr) {
    frame_mark_free(addr / FRAME_SIZE);
}

uint32_t pmm_total_frames(void) { return total_frames; }
uint32_t pmm_used_frames(void)  { return used_frames; }
uint32_t pmm_free_frames(void)  { return total_frames - used_frames; }
uint32_t pmm_bitmap_addr(void)  { return (uint32_t) bitmap; }
uint32_t pmm_bitmap_size(void)  { return bitmap_size; }
