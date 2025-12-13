#pragma once

#include <cstdint>

/*
 * Legacy PIT timer, only used by the APIC timer to calibrate its frequency and then never used again.
 */
namespace i686::drivers::pit {
    inline constexpr std::uint8_t CHANNEL_0_DATA_PORT = 0x40;
    inline constexpr std::uint8_t CHANNEL_1_DATA_PORT = 0x41;
    inline constexpr std::uint8_t CHANNEL_2_DATA_PORT = 0x42;
    inline constexpr std::uint8_t COMMAND_REGISTER    = 0x43;

    inline constexpr std::uint32_t BASE_FREQUENCY     = 1193182; // Hz

    inline constexpr std::uint8_t CMD_CHANNEL_0       = 0x00;
    inline constexpr std::uint8_t CMD_ACCESS_LOHI     = 0x30;  // Access mode: lobyte/hibyte
    inline constexpr std::uint8_t CMD_MODE_0          = 0x00;  // Mode 0: Interrupt on terminal count (one-shot)
    inline constexpr std::uint8_t CMD_MODE_2          = 0x04;  // Mode 2: Rate generator
    inline constexpr std::uint8_t CMD_BINARY          = 0x00;  // Binary mode

    void sleep_ms(std::uint32_t ms);
}
