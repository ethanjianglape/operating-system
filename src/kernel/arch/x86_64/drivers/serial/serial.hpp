#pragma once

#include <kernel/console/console.hpp>

#include <cstdint>

namespace x86_64::drivers::serial {
    constexpr std::uint16_t COM1 = 0x3F8;

    // UART register offsets
    constexpr std::uint16_t DATA        = 0; // Data register (read/write)
    constexpr std::uint16_t INT_ENABLE  = 1; // Interrupt enable
    constexpr std::uint16_t FIFO_CTRL   = 2; // FIFO control
    constexpr std::uint16_t LINE_CTRL   = 3; // Line control
    constexpr std::uint16_t MODEM_CTRL  = 4; // Modem control
    constexpr std::uint16_t LINE_STATUS = 5; // Line status

    // Line status bits
    constexpr std::uint8_t LSR_TRANSMIT_EMPTY = 0x20;

    void init();
    void putchar(char c);
    void puts(const char* str);

    kernel::console::ConsoleDriver* get_console_driver();
}
