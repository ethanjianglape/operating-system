#include <string.h>

size_t strlen(const char* str) {
    size_t len = 0;

    while (str[len]) {
        len++;
    }

    return len;
}

int strcmp(const char* str1, const char* str2) {
    if (strlen(str1) != strlen(str2)) {
        return 1;
    }
    
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return 1;
        }

        str1++;
        str2++;
    }

    return 0;
}
