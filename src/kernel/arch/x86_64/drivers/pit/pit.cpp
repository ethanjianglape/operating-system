#include "pit.hpp"
#include "arch/x86_64/cpu/cpu.hpp"

namespace x86_64::drivers::pit {
    void sleep_ms(std::uint32_t ms) {
        const std::uint8_t command = CMD_CHANNEL_0 | CMD_ACCESS_LOHI | CMD_MODE_0 | CMD_BINARY;
        const std::uint16_t divisor = (BASE_FREQUENCY * ms) / 1000;
        const std::uint8_t low = divisor & 0xFF;
        const std::uint8_t high = (divisor >> 8) & 0xFF;

        cpu::outb(COMMAND_REGISTER, command);
        cpu::outb(CHANNEL_0_DATA_PORT, low);
        cpu::outb(CHANNEL_0_DATA_PORT, high);

        while (true) {
            cpu::outb(COMMAND_REGISTER, 0xE2);
            const auto data = cpu::inb(CHANNEL_0_DATA_PORT);

            if (data & 0x80) {
                break;
            }
        }
    }
}
