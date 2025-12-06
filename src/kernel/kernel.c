#include <stdio.h>

#include <kernel/tty.h>

void kernel_main(void) {
    terminal_initialize();

    printf("~~ My OS 2! ~~\n");

    // Infinite loop - kernel should never exit
    while (1) {
        asm volatile("hlt");
    }
}
