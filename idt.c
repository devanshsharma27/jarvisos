#include "idt.h"

#define IDT_ENTRIES 256

struct idt_entry idt[IDT_ENTRIES];   // the actual table: 256 entries
struct idt_ptr   idtp;               // the pointer the CPU loads

// Implemented in assembly (idt_load.asm) — runs the LIDT instruction
extern void idt_load(uint32_t);

// Fill in one entry of the table
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector  = selector;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

void idt_install(void) {
    // Set up the pointer: limit = total size - 1, base = table address
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base  = (uint32_t) &idt;

    // Zero out every entry to start clean
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].base_low  = 0;
        idt[i].base_high = 0;
        idt[i].selector  = 0;
        idt[i].always0   = 0;
        idt[i].flags     = 0;
    }

    // Tell the CPU to use this table
    idt_load((uint32_t) &idtp);
}
