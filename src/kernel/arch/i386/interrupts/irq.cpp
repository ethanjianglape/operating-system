#include "irq.hpp"
#include "kernel/tty.h"
#include "arch/i386/cpu/cpu.hpp"
#include <cstdint>

using namespace i386;

static irq::irq_handler_t irq_handlers[256] = {nullptr};

void irq::register_irq_handler(const std::uint32_t vector, irq::irq_handler_t handler) {
    if (vector >= 32 && vector <= 255) {
        irq_handlers[vector] = handler;
    }
}

[[noreturn]]
void handle_exception(const std::uint32_t vector, const std::uint32_t error) {
    terminal_clear();
    terminal_puts("~~ FATAL EXCEPTION ~~\n");
    terminal_puts("vector = ");
    terminal_putint(vector);
    terminal_puts("\n");
    terminal_puts("error = ");
    terminal_putint(error);
    terminal_puts("\n");
    terminal_puts("Kernel Panic!\n");

    cpu::cli();

    while (true) {
        cpu::hlt();
    }
}

void handle_irq(const std::uint32_t vector) {
    if (irq_handlers[vector]) {
        irq_handlers[vector](vector);
    }
}

extern "C"
void interrupt_handler(const std::uint32_t vector, const std::uint32_t error) {
    if (vector < 32) {
        handle_exception(vector, error);
    } else {
        handle_irq(vector);
    }
}
