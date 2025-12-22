#include "keyboard.hpp"
#include "arch/x86_64/interrupts/irq.hpp"

#include <cstdint>
#include <kernel/log/log.hpp>

namespace x86_64::drivers::keyboard {
    void keyboard_interrupt_handler(std::uint32_t vector) {
        kernel::log::info("IRQ1: %d", vector);
    }
    
    void init() {
        kernel::log::init_start("Keyboard");

        x86_64::irq::register_irq_handler(32, keyboard_interrupt_handler);

        kernel::log::init_end("Keyboard");
    }
}
