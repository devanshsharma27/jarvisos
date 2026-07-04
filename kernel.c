#include "vga.h"
#include "idt.h"

void kernel_main(void) {
    clear_screen();
    print("Welcome to JarvisOS!\n");

    print("Installing IDT... ");
    idt_install();
    print("done.\n");

    print("IDT loaded. CPU now knows where its interrupt table is.\n");
    print("Next: PIC remapping and the keyboard handler.\n");

    while (1) { __asm__("hlt"); }
}
