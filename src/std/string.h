#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include "stddef.h"

int strcmp(const char *a, const char *b);
char* strcpy(char *dest, const char *src);
void *memset(void *s, int c, size_t n);

#endif // STRING_H