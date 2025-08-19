#include "std/stddef.h"

int strcmp(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return (*a == *b);
}

char* strcpy(char *dest, const char *src) {
    char *original = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}