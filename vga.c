#include <stdint.h>
#include <stddef.h>
#include "vga.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static volatile uint16_t* vga = (uint16_t*) 0xB8000;
static size_t cursor_row = 0;
static size_t cursor_col = 0;
static uint8_t color = 0x0F;

static uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

void clear_screen(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = vga_entry(' ', color);
    cursor_row = 0;
    cursor_col = 0;
}

static void scroll(void) {
    for (size_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++)
        vga[i] = vga[i + VGA_WIDTH];
    for (size_t x = 0; x < VGA_WIDTH; x++)
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', color);
    cursor_row = VGA_HEIGHT - 1;
}

void putchar(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else {
        vga[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(c, color);
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }
    if (cursor_row >= VGA_HEIGHT)
        scroll();
}

void print(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++)
        putchar(str[i]);
}

void print_int(int n) {
    if (n == 0) { putchar('0'); return; }
    char buf[12];
    int len = 0;
    if (n < 0) { putchar('-'); n = -n; }
    while (n > 0) { buf[len++] = '0' + (n % 10); n /= 10; }
    while (len > 0) putchar(buf[--len]);
}
void backspace(void) {
    if (cursor_col == 0) return;   // don't erase past line start
    cursor_col--;
    vga[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(' ', color);
}
