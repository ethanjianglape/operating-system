#include "pic.hpp"

#include "arch/x86_64/cpu/cpu.hpp"
#include "kernel/log/log.hpp"
#include "kernel/panic/panic.hpp"

namespace x86_64::drivers::pic {
    bool init() {
        kernel::log::init_start("Legacy PIT");
        
        cpu::outb(PIC1_DATA, 0xFF);
        cpu::outb(PIC2_DATA, 0xFF);

        const auto master = cpu::inb(PIC1_DATA);
        const auto slave = cpu::inb(PIC2_DATA);
        const auto success = master == 0xFF && slave == 0xFF;

        if (!success) {
            kernel::panic("Failed to disable legacy PIT");
        }

        kernel::log::info("Legacy PIT has been disabled");
        kernel::log::init_end("Legacy PIT");

        return success;
    }
}
