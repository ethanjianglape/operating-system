#include <crt/crt.h>

int memcmp(const void* v1, const void* v2, size_t count) {
    const unsigned char* p1 = (const unsigned char*)v1;
    const unsigned char* p2 = (const unsigned char*)v2;
    
    while (count--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }

        p1++;
        p2++;
    }

    return 0;
}

void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    while (count--) {
        *d++ = *s++;
    }

    return dest;
}

void* memmove(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    if (d < s) {
        while (count--) {
            *d++ = *s++;
        }
    } else {
        d += count;
        s += count;

        while (count--) {
            *--d = *--s;
        }
    }

    return dest;
}

void* memset(void* dest, int ch, size_t count) {
    char* ptr = (char*)dest;
    unsigned char c = (unsigned char)ch;

    for (size_t i = 0; i < count; i++) {
        ptr[i] = c;
    }

    return ptr;
}

size_t strlen(const char* str) {
    size_t len = 0;

    while (*str++) {
        len++;
    }

    return len;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && *str1 == *str2) {
        str1++;
        str2++;
    }

    return *str1 - *str2;
}
