#pragma once

#include <cstdint>

namespace x86_64::drivers::pic {
    constexpr std::uint16_t PIC1_COMMAND = 0x20;
    constexpr std::uint16_t PIC1_DATA = 0x21;
    constexpr std::uint16_t PIC2_COMMAND = 0xA0;
    constexpr std::uint16_t PIC2_DATA = 0xA1;
    
    bool init();
}
