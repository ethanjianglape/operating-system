#include "pic.hpp"

#include <arch/x86_64/cpu/cpu.hpp>
#include <log/log.hpp>
#include <panic/panic.hpp>

namespace x86_64::drivers::pic {
    bool init() {
        log::init_start("Legacy PIT");
        
        cpu::outb(PIC1_DATA, 0xFF);
        cpu::outb(PIC2_DATA, 0xFF);

        const auto master = cpu::inb(PIC1_DATA);
        const auto slave = cpu::inb(PIC2_DATA);
        const auto success = master == 0xFF && slave == 0xFF;

        if (!success) {
            panic("Failed to disable legacy PIT");
        }

        log::info("Legacy PIT has been disabled");
        log::init_end("Legacy PIT");

        return success;
    }
}
