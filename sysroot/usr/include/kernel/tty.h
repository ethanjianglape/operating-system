#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void terminal_initialize(void);
void terminal_clear(void);
void terminal_scroll(void);
void terminal_newline(void);
void terminal_putchar(char c);
void terminal_putint(int i);
void terminal_putuint(uint32_t i);
void terminal_puts(const char* str);
void terminal_write(const char* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
