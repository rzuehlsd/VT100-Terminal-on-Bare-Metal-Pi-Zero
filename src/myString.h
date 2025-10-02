#ifndef MYSTRING_H
#define MYSTRING_H

#include <stddef.h>

size_t strlen(const char* s);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);

#endif // MYSTRING_H