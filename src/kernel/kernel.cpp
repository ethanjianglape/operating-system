#include "vga.hpp"

extern "C" [[noreturn]]
void kernel_main() {
    vga::clear_screen();

    // Print welcome message
    vga::print_str("~~~ Welcome to MyOS! ~~~\n", vga::Color::LIGHT_CYAN);
    vga::print_str("Kernel initialized successfully.\n", vga::Color::LIGHT_GREEN);
    vga::print_str("\n");

    vga::printf("this is my string %d | %d", 123, 984);

    for (int i = 0; i < 1000; i++) {
        vga::printf("i == %d\n", i);
    }

    // Infinite loop - kernel should never exit
    while (1) {
        asm volatile("hlt");
    }
}
