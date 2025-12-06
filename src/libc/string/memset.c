#include <string.h>

void* memset(void* destptr, int value, size_t len) {
    unsigned char* dest = destptr;
    
    for (size_t i = 0; i < len; i++) {
        dest[i] = value;
    }

    return dest;
}
