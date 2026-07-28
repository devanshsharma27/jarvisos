#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

int strcmp(const char* a, const char* b);
int strlen(const char* s);
int starts_with(const char* str, const char* prefix);
int atoi(const char* s);
uint32_t atoi_hex(const char* s);
void* memset(void* dest, int val, uint32_t count);

#endif
