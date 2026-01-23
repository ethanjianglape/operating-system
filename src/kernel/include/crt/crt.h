#pragma once

// C Runtime (crt) functions required by GCC and ACPICA

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory functions
int memcmp(const void*, const void*, size_t);
void* memcpy(void* dest, const void* src, size_t count);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);

// String functions
size_t strlen(const char*);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);

// Character classification (ctype)
int isprint(int c);
int isspace(int c);
int isdigit(int c);
int isxdigit(int c);
int isupper(int c);
int islower(int c);
int isalpha(int c);
int toupper(int c);
int tolower(int c);

#ifdef __cplusplus
}
#endif
