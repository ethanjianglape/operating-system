/**
 * @file limine_entry.cpp
 * @brief Limine bootloader entry point.
 *
 * This is where the Limine bootloader transfers control to the kernel. By this
 * point, Limine has already set up 64-bit long mode, the HHDM (Higher Half
 * Direct Map), and provided boot info via its protocol (memory map, framebuffer,
 * RSDP, etc.). We just call kernel_main() and hang if it ever returns.
 *
 * Other bootloaders (GRUB, custom) would have their own entry files with
 * different setup requirements — Limine handles more for us than most.
 */

#include <arch/x86_64/cpu/cpu.hpp>

extern void kernel_main();

extern "C" [[noreturn]]
void _start(void) {
    kernel_main();

    while (true) {
        x86_64::cpu::cli();
        x86_64::cpu::hlt();
    }
}
