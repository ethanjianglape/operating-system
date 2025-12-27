#pragma once

// C Runtime (crt) functions required by GCC

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void*, const void*, size_t);
void* memcpy(void* dest, const void* src, size_t count);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
size_t strlen(const char*);
int strcmp(const char* str1, const char* str2);

#ifdef __cplusplus
}
#endif
