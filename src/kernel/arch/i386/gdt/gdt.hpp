#pragma once

#include <cstdint>

namespace i386::gdt {
    inline constexpr std::uint8_t FLAG_GRANULARITY = 0x8;
    inline constexpr std::uint8_t FLAG_32BIT       = 0x4;
    inline constexpr std::uint8_t FLAG_64BIT       = 0x2;
    inline constexpr std::uint8_t FLAG_AVL         = 0x1;

    inline constexpr std::uint8_t FLAGS_32BIT_4KB = FLAG_GRANULARITY | FLAG_32BIT;

    inline constexpr uint8_t ACCESS_PRESENT    = 0x80;  // 1 << 7
    inline constexpr uint8_t ACCESS_RING_0     = 0x00;  // DPL = 0
    inline constexpr uint8_t ACCESS_RING_3     = 0x60;  // DPL = 3
    inline constexpr uint8_t ACCESS_CODE_DATA  = 0x10;  // S bit
    inline constexpr uint8_t ACCESS_EXECUTABLE = 0x08;  // Code segment
    inline constexpr uint8_t ACCESS_READABLE   = 0x02;  // Code can be read
    inline constexpr uint8_t ACCESS_WRITABLE   = 0x02;  // Data can be written

    inline constexpr uint8_t KERNEL_CODE = ACCESS_PRESENT | ACCESS_RING_0 |
                                           ACCESS_CODE_DATA | ACCESS_EXECUTABLE | ACCESS_READABLE;

    inline constexpr uint8_t KERNEL_DATA = ACCESS_PRESENT | ACCESS_RING_0 |
                                           ACCESS_CODE_DATA | ACCESS_WRITABLE;

    inline constexpr uint8_t USER_CODE   = ACCESS_PRESENT | ACCESS_RING_3 |
                                           ACCESS_CODE_DATA | ACCESS_EXECUTABLE | ACCESS_READABLE;

    inline constexpr uint8_t USER_DATA   = ACCESS_PRESENT | ACCESS_RING_3 |
                                           ACCESS_CODE_DATA | ACCESS_WRITABLE;

    
    struct [[gnu::packed]] gdt_entry {
        std::uint16_t limit_low;
        std::uint16_t base_low;
        std::uint8_t base_mid;
        std::uint8_t access;
        std::uint8_t limit_mid : 4;
        std::uint8_t flags     : 4;
        std::uint8_t base_high;
    };

    static_assert(sizeof(gdt_entry) == 8, "GDT entry must be 8 bytes");

    struct [[gnu::packed]] gdt_ptr {
        std::uint16_t limit;
        std::uint32_t base;
    };

    void init();
}
