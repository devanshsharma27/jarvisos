#include "paging.h"
#include "pmm.h"
#include "memory.h"
#include "vga.h"
#include "util.h"

// A 32-bit virtual address splits into three fields:
//
//   31            22 21           12 11              0
//  +----------------+---------------+-----------------+
//  | directory index|  table index  |  offset in page |
//  +----------------+---------------+-----------------+
//
// The directory holds 1024 tables, each table holds 1024 pages of 4 KB,
// so one directory can describe the whole 4 GB address space.

#define DIR_INDEX(v)   ((v) >> 22)
#define TBL_INDEX(v)   (((v) >> 12) & 0x3FF)
#define FRAME_OF(e)    ((e) & 0xFFFFF000)

// How much physical memory we map one-to-one at boot. The kernel, the frame
// bitmap, the page tables and the VGA buffer all live down here, so while
// paging is on we can still reach them at their physical addresses.
#define IDENTITY_BYTES (16 * 1024 * 1024)

static uint32_t* page_directory = 0;
static uint32_t  dir_phys       = 0;
static uint32_t  identity_end   = 0;
static uint32_t  table_count    = 0;
static uint32_t  mapped_pages   = 0;
static int       enabled        = 0;

static inline void flush_tlb_page(uint32_t virt) {
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

static inline uint32_t read_cr2(void) {
    uint32_t v;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(v));
    return v;
}

// Find the page table for this address, optionally creating it.
// Page tables are ordinary 4 KB frames taken from the physical allocator.
static uint32_t* get_table(uint32_t virt, int create) {
    uint32_t di = DIR_INDEX(virt);

    if (page_directory[di] & PAGE_PRESENT)
        return (uint32_t*) FRAME_OF(page_directory[di]);

    if (!create) return 0;

    uint32_t frame = pmm_alloc_frame();
    if (frame == 0) return 0;

    // We reach page tables through the identity map, so a table living above
    // the identity window would be unreachable the moment paging comes on.
    if (frame >= identity_end) {
        pmm_free_frame(frame);
        return 0;
    }

    memset((void*) frame, 0, PAGE_SIZE);
    page_directory[di] = frame | PAGE_PRESENT | PAGE_WRITE;
    table_count++;
    return (uint32_t*) frame;
}

int paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t* table = get_table(virt, 1);
    if (!table) return 0;

    uint32_t ti = TBL_INDEX(virt);
    if (!(table[ti] & PAGE_PRESENT)) mapped_pages++;

    table[ti] = FRAME_OF(phys) | (flags & 0xFFF) | PAGE_PRESENT;
    flush_tlb_page(virt);
    return 1;
}

int paging_unmap(uint32_t virt) {
    uint32_t* table = get_table(virt, 0);
    if (!table) return 0;

    uint32_t ti = TBL_INDEX(virt);
    if (!(table[ti] & PAGE_PRESENT)) return 0;

    table[ti] = 0;
    mapped_pages--;
    flush_tlb_page(virt);
    return 1;
}

// Walk the tables by hand, exactly like the MMU does in hardware.
uint32_t paging_translate(uint32_t virt) {
    uint32_t* table = get_table(virt, 0);
    if (!table) return PAGE_UNMAPPED;

    uint32_t entry = table[TBL_INDEX(virt)];
    if (!(entry & PAGE_PRESENT)) return PAGE_UNMAPPED;

    return FRAME_OF(entry) | (virt & 0xFFF);
}

void paging_init(void) {
    identity_end = IDENTITY_BYTES;
    uint32_t highest = mem_highest_addr();
    if (highest && highest < identity_end)
        identity_end = highest & ~(PAGE_SIZE - 1);

    dir_phys = pmm_alloc_frame();
    if (dir_phys == 0) {
        print("FATAL: no frame for the page directory.\n");
        return;
    }
    page_directory = (uint32_t*) dir_phys;
    memset(page_directory, 0, PAGE_SIZE);

    // Identity map: virtual address == physical address, kernel-only, writable.
    for (uint32_t addr = 0; addr < identity_end; addr += PAGE_SIZE) {
        if (!paging_map(addr, addr, PAGE_WRITE)) {
            print("FATAL: ran out of frames building the identity map.\n");
            return;
        }
    }

    // CR3 points at the directory; setting bit 31 of CR0 turns the MMU on.
    // From the next instruction onward every address goes through the tables.
    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or  $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : "r"(dir_phys) : "eax", "memory"
    );
    enabled = 1;
}

// Interrupt 14. CR2 holds the address that faulted; the error code says why.
void page_fault_handler(uint32_t err_code) {
    uint32_t addr = read_cr2();

    print("\n*** PAGE FAULT ***\n");
    print("  address : ");
    print_hex(addr);
    print("\n  cause   : ");
    print((err_code & 0x1) ? "protection violation" : "page not present");
    print(", ");
    print((err_code & 0x2) ? "on write" : "on read");
    print(", ");
    print((err_code & 0x4) ? "user mode" : "kernel mode");
    if (err_code & 0x10) print(", instruction fetch");
    print("\n  code    : ");
    print_hex(err_code);
    print("\n");
}

int      paging_is_enabled(void)   { return enabled; }
uint32_t paging_dir_phys(void)     { return dir_phys; }
uint32_t paging_identity_end(void) { return identity_end; }
uint32_t paging_table_count(void)  { return table_count; }
uint32_t paging_mapped_pages(void) { return mapped_pages; }
