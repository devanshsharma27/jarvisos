#include <stdint.h>
#include "vga.h"
#include "idt.h"
#include "isr.h"
#include "shell.h"
#include "memory.h"
#include "multiboot.h"

void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
    clear_screen();
    print("JarvisOS 0.3.0 booting...\n");

    if (magic != MULTIBOOT_MAGIC) {
        print("FATAL: not booted by a Multiboot loader.\n");
        while (1) { __asm__("hlt"); }
    }

    memory_init(mbi);
    print("Memory map parsed: ");
    print_int(mem_usable_kb() / 1024);
    print(" MB usable RAM detected.\n");

    idt_install();
    isr_install();
    __asm__ volatile ("sti");
    print("Interrupts online. Keyboard ready.\n");
    print("Type 'help' for commands.\n");

    shell_init();
    while (1) { __asm__("hlt"); }
}
