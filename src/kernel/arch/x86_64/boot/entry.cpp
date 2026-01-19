/**
 * @file entry.cpp
 * @brief Limine bootloader entry point.
 *
 * This is where the bootloader transfers control to the kernel. Limine has
 * already set up 64-bit long mode, the HHDM, and provided us with boot info
 * (memory map, framebuffer, etc.). We just call kernel_main() and hang if
 * it ever returns.
 */

#include <arch/x86_64/cpu/cpu.hpp>

extern void kernel_main();

extern "C" [[noreturn]]
void _start(void) {
    kernel_main();

    while (true) {
        x86_64::cpu::hlt();
    }
}
