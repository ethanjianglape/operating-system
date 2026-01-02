#pragma once

#include <cstdint>

/**
 * Legacy 8254 PIT (Programmable Interval Timer)
 *
 * Only used to calibrate the APIC timer, then never touched again.
 * See pit.cpp for detailed documentation.
 */
namespace x86_64::drivers::pit {
    // I/O ports for the three PIT channels and command register
    constexpr std::uint8_t CHANNEL_0_DATA_PORT = 0x40;
    constexpr std::uint8_t CHANNEL_1_DATA_PORT = 0x41;  // Historically for DRAM refresh
    constexpr std::uint8_t CHANNEL_2_DATA_PORT = 0x42;  // Connected to PC speaker
    constexpr std::uint8_t COMMAND_REGISTER    = 0x43;

    // The PIT's oscillator frequency - exactly 1.193182 MHz on ALL PCs since 1981.
    // This is derived from the original PC's 14.31818 MHz crystal ÷ 12.
    // It's a weird number, but it's been frozen in time for backwards compatibility.
    constexpr std::uint32_t BASE_FREQUENCY     = 1193182;
    
    constexpr std::uint8_t CMD_CHANNEL_0       = 0x00;
    constexpr std::uint8_t CMD_ACCESS_LOHI     = 0x30;  // Access mode: lobyte/hibyte
    constexpr std::uint8_t CMD_MODE_0          = 0x00;  // Mode 0: Interrupt on terminal count (one-shot)
    constexpr std::uint8_t CMD_MODE_2          = 0x04;  // Mode 2: Rate generator
    constexpr std::uint8_t CMD_BINARY          = 0x00;  // Binary mode
    
    void sleep_ms(std::uint32_t ms);
}
