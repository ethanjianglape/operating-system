#pragma once

#include <cstdint>

namespace i686::gdt {
    inline constexpr std::uint8_t FLAG_GRANULARITY = 0x8;
    inline constexpr std::uint8_t FLAG_32BIT       = 0x4;
    inline constexpr std::uint8_t FLAG_64BIT       = 0x2;
    inline constexpr std::uint8_t FLAG_AVL         = 0x1;

    inline constexpr std::uint8_t FLAGS_32BIT_4KB = FLAG_GRANULARITY | FLAG_32BIT;

    inline constexpr std::uint8_t TSS_ACCESS   = 0x89;

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

    struct [[gnu::packed]] tss_entry {
        std::uint32_t prev_tss;   // Previous TSS (unused in modern OSes)
        std::uint32_t esp0;       // Stack pointer for ring 0
        std::uint32_t ss0;        // Stack segment for ring 0
        std::uint32_t esp1;       // Stack pointer for ring 1 (rarely used)
        std::uint32_t ss1;        // Stack segment for ring 1
        std::uint32_t esp2;       // Stack pointer for ring 2 (rarely used)
        std::uint32_t ss2;        // Stack segment for ring 2
        std::uint32_t cr3;        // Page directory base (for hardware task switching)
        std::uint32_t eip;
        std::uint32_t eflags;
        std::uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
        std::uint32_t es, cs, ss, ds, fs, gs;
        std::uint32_t ldtr;       // LDT selector (unused)
        std::uint16_t trap;
        std::uint16_t iomap_base; // I/O permission bitmap
    };

    void init();
}
