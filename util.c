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

// Does str start with prefix?
int starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

// Parse an integer from a string (supports negative)
int atoi(const char* s) {
    int n = 0, sign = 1;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n * sign;
}
