#include <arch/x86_64/cpu/cpu.hpp>

extern void kernel_main();

[[noreturn]]
void _start(void) {
    kernel_main();

    // Hang if kernel_main() ever returns
    while (true) {
        x86_64::cpu::hlt();
    }
}
