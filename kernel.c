#include "vga.h"
#include "idt.h"
#include "isr.h"
#include "shell.h"

void kernel_main(void) {
    clear_screen();
    print("JarvisOS 0.2.0 booting...\n");

    idt_install();
    isr_install();
    __asm__ volatile ("sti");

    print("Interrupts online. Keyboard ready.\n");
    print("Type 'help' for commands.\n");

    shell_init();

    while (1) { __asm__("hlt"); }
}
