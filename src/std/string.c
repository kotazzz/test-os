#include "std/stddef.h"

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

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