#pragma once

#include <cstdint>

/*
 * Legacy PIT timer, only used by the APIC timer to calibrate its frequency and then never used again.
 */
namespace x86_64::drivers::pit {
    constexpr std::uint8_t CHANNEL_0_DATA_PORT = 0x40;
    constexpr std::uint8_t CHANNEL_1_DATA_PORT = 0x41;
    constexpr std::uint8_t CHANNEL_2_DATA_PORT = 0x42;
    constexpr std::uint8_t COMMAND_REGISTER    = 0x43;
    
    constexpr std::uint32_t BASE_FREQUENCY     = 1193182; // Hz
    
    constexpr std::uint8_t CMD_CHANNEL_0       = 0x00;
    constexpr std::uint8_t CMD_ACCESS_LOHI     = 0x30;  // Access mode: lobyte/hibyte
    constexpr std::uint8_t CMD_MODE_0          = 0x00;  // Mode 0: Interrupt on terminal count (one-shot)
    constexpr std::uint8_t CMD_MODE_2          = 0x04;  // Mode 2: Rate generator
    constexpr std::uint8_t CMD_BINARY          = 0x00;  // Binary mode
    
    void sleep_ms(std::uint32_t ms);
}
