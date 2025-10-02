#include "myString.h"


size_t strlen(const char* s) {
    size_t n = 0;
    while (s && *s++) n++;
    return n;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;
    size_t i = 0;
    while (i < n && src[i]) {
        d[i] = src[i];
        i++;
    }
    while (i < n) {
        d[i++] = '\0';
    }
    return dest;
}