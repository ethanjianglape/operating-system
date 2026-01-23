#include <crt/crt.h>

// =============================================================================
// Character Classification (ctype)
// =============================================================================

int isprint(int c) {
    return (c >= 0x20 && c <= 0x7E);
}

int isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' ||
            c == '\r' || c == '\f' || c == '\v');
}

int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

int isxdigit(int c) {
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int isupper(int c) {
    return (c >= 'A' && c <= 'Z');
}

int islower(int c) {
    return (c >= 'a' && c <= 'z');
}

int isalpha(int c) {
    return isupper(c) || islower(c);
}

// =============================================================================
// glibc ctype compatibility
// These are internal glibc functions that ctype.h macros call.
// =============================================================================

// Character class bits used by glibc
#define _ISupper  256
#define _ISlower  512
#define _ISalpha  1024
#define _ISdigit  2048
#define _ISxdigit 4096
#define _ISspace  8192
#define _ISprint  16384
#define _ISgraph  32768
#define _ISblank  1
#define _IScntrl  2
#define _ISpunct  4
#define _ISalnum  8

static unsigned short ctype_table[384];
static int toupper_table[384];
static int tolower_table[384];
static int ctype_initialized = 0;

static void init_ctype_tables(void) {
    if (ctype_initialized) return;

    for (int i = 0; i < 384; i++) {
        int c = i - 128;
        ctype_table[i] = 0;
        toupper_table[i] = c;
        tolower_table[i] = c;

        if (c >= 0 && c <= 127) {
            if (c >= 'A' && c <= 'Z') {
                ctype_table[i] |= _ISupper | _ISalpha | _ISprint | _ISgraph | _ISalnum;
                if (c <= 'F') ctype_table[i] |= _ISxdigit;
                tolower_table[i] = c - 'A' + 'a';
            }
            if (c >= 'a' && c <= 'z') {
                ctype_table[i] |= _ISlower | _ISalpha | _ISprint | _ISgraph | _ISalnum;
                if (c <= 'f') ctype_table[i] |= _ISxdigit;
                toupper_table[i] = c - 'a' + 'A';
            }
            if (c >= '0' && c <= '9') {
                ctype_table[i] |= _ISdigit | _ISxdigit | _ISprint | _ISgraph | _ISalnum;
            }
            if (c == ' ' || c == '\t') {
                ctype_table[i] |= _ISspace | _ISblank | _ISprint;
            }
            if (c == '\n' || c == '\r' || c == '\f' || c == '\v') {
                ctype_table[i] |= _ISspace;
            }
            if (c >= 0x21 && c <= 0x7e && !((ctype_table[i] & (_ISalnum)))) {
                ctype_table[i] |= _ISpunct | _ISprint | _ISgraph;
            }
            if (c == ' ') {
                ctype_table[i] |= _ISprint;
            }
            if (c < 0x20 || c == 0x7f) {
                ctype_table[i] |= _IScntrl;
            }
        }
    }
    ctype_initialized = 1;
}

static const unsigned short *ctype_b_ptr;
static const int *toupper_ptr;
static const int *tolower_ptr;

const unsigned short **__ctype_b_loc(void) {
    init_ctype_tables();
    ctype_b_ptr = &ctype_table[128];
    return &ctype_b_ptr;
}

const int **__ctype_toupper_loc(void) {
    init_ctype_tables();
    toupper_ptr = &toupper_table[128];
    return &toupper_ptr;
}

const int **__ctype_tolower_loc(void) {
    init_ctype_tables();
    tolower_ptr = &tolower_table[128];
    return &tolower_ptr;
}
