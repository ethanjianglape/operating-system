#include "vga.hpp"

extern "C" [[noreturn]]
void kernel_main() {
    vga::initialize();

    // Print welcome message
    vga::print_str("~~~ Welcome to MyOS! ~~~\n", vga::Color::LIGHT_CYAN);
    //vga::print_str("Kernel initialized successfully.\n", vga::Color::LIGHT_GREEN);
    //vga::print_str("\n");

    //vga::printf("this is my string %d | %d", 123, 984);    

    // Infinite loop - kernel should never exit
    while (1) {
        asm volatile("hlt");
    }
}
