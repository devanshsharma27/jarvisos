#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// One entry in the IDT. Must be packed to exactly 8 bytes,
// because the CPU reads this layout directly from memory.
struct idt_entry {
    uint16_t base_low;   // lower 16 bits of handler address
    uint16_t selector;   // code segment selector (from our GDT)
    uint8_t  always0;    // reserved, always zero
    uint8_t  flags;      // type and attributes
    uint16_t base_high;  // upper 16 bits of handler address
} __attribute__((packed));

// Tells the CPU where the IDT is and how big it is.
struct idt_ptr {
    uint16_t limit;      // size of the table minus 1
    uint32_t base;       // address of the first entry
} __attribute__((packed));

void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);
void idt_install(void);

#endif
