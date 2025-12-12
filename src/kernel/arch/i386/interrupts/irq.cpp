#include "irq.hpp"
#include "kernel/kprintf/kprintf.hpp"
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
    kernel::kprintf("~~ FATAL EXCEPTION ~~\n");
    kernel::kprintf("vector = %d\n", vector);
    kernel::kprintf("error = %d\n", error);
    kernel::kprintf("Kernel Panic!\n");

    // page faults
    if (vector == 14) {
        std::uint32_t addr;
        asm volatile("mov %%cr2, %0" : "=r"(addr));
        kernel::kprintf("page fault at addr = %d\n", addr);
    }

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
    kernel::kprintf("[IRQ %d]\n", vector);

    if (vector < 32) {
        handle_exception(vector, error);
    } else {
        handle_irq(vector);
    }
}
