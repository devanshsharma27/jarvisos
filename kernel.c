#include <stdint.h>

volatile uint16_t* vga = (uint16_t*) 0xB8000;

uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

void kernel_main(void) {
    const char* msg = "Welcome to JarvisOS!";
    uint8_t color = 0x0F;

    for (int i = 0; i < 80 * 25; i++)
        vga[i] = vga_entry(' ', color);

    for (int i = 0; msg[i] != '\0'; i++)
        vga[i] = vga_entry(msg[i], color);

    while (1) { __asm__("hlt"); }
}
