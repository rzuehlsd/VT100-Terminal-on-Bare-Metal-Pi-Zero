//
// c_utils.h
// Several general functions
//
// PiGFX is a bare metal kernel for the Raspberry Pi
// that implements a basic ANSI terminal emulator with
// the additional support of some primitive graphics functions.
// Copyright (C) 2020 Christian Lehner

#ifndef C_UTILS_H__
#define C_UTILS_H__

#include <stdint.h>
#include <stddef.h>

void *pivt100_memset (void *pBuffer, int nValue, size_t nLength);
void *qmemcpy(void *dest, void *src, size_t n);
void veryfastmemcpy(void *dest, void* src, unsigned int n);
void *pivt100_memcpy (void *pDest, const void *pSrc, size_t nLength);
char *pivt100_strcpy (char *pDest, const char *pSrc);
size_t pivt100_strlen (const char *pString);
int pivt100_strcmp (const char *pString1, const char *pString2);
int isspace(int c);
char *pivt100_strncpy (char *pDest, const char *pSrc, size_t nMaxLen);
char *strchr(const char *p, int ch);
int32_t atoi(const char *p);

#define memset(a,b,c) pivt100_memset(a,b,c)
#define memcpy(a,b,c) pivt100_memcpy(a,b,c)

#endif
