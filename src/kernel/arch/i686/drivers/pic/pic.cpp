#include "pic.hpp"

#include "arch/i686/cpu/cpu.hpp"
#include "kernel/log/log.hpp"

namespace i686::drivers::pic {
    constexpr std::uint16_t PIC1_COMMAND = 0x20;
    constexpr std::uint16_t PIC1_DATA = 0x21;
    constexpr std::uint16_t PIC2_COMMAND = 0xA0;
    constexpr std::uint16_t PIC2_DATA = 0xA1;

    bool init() {
        cpu::outb(PIC1_DATA, 0xFF);
        cpu::outb(PIC2_DATA, 0xFF);

        const auto master = cpu::inb(PIC1_DATA);
        const auto slave = cpu::inb(PIC2_DATA);

        kernel::log::success("Legacy PIT Disabled");

        return master == 0xFF && slave == 0xFF;
    }
}
