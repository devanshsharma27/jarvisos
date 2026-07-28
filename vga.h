#ifndef VGA_H
#define VGA_H

#include <stdint.h>

void clear_screen(void);
void print(const char* str);
void print_int(int n);
void print_hex(uint32_t n);
void putchar(char c);
void backspace(void);

#endif
