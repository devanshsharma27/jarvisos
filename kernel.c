#include "vga.h"

void kernel_main(void) {
    clear_screen();
    print("Welcome to JarvisOS!\n");
    print("Now built from multiple files with a Makefile.\n");
    print("Screen driver lives in vga.c\n");
    print("Ready to add keyboard support next.\n");

    while (1) { __asm__("hlt"); }
}
