#include "util.h"

int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int strlen(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

int starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

int atoi(const char* s) {
    int n = 0, sign = 1;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n * sign;
}

// Parse "0x1234" or "1234" as hexadecimal.
uint32_t atoi_hex(const char* s) {
    uint32_t n = 0;
    while (*s == ' ') s++;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (*s) {
        char c = *s;
        uint32_t d;
        if      (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else break;
        n = n * 16 + d;
        s++;
    }
    return n;
}

void* memset(void* dest, int val, uint32_t count) {
    unsigned char* p = (unsigned char*) dest;
    while (count--) *p++ = (unsigned char) val;
    return dest;
}
