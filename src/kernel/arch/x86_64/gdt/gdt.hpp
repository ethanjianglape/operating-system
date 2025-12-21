#pragma once

#include <cstdint>

namespace x86_64::gdt {
    inline constexpr std::uint8_t FLAG_GRANULARITY = 0x8;
    inline constexpr std::uint8_t FLAG_32BIT       = 0x4;
    inline constexpr std::uint8_t FLAG_64BIT       = 0x2;
    inline constexpr std::uint8_t FLAG_AVL         = 0x1;

    inline constexpr std::uint8_t FLAGS_32BIT_4KB = FLAG_GRANULARITY | FLAG_32BIT;
    inline constexpr std::uint8_t FLAGS_64BIT_4KB = FLAG_GRANULARITY | FLAG_64BIT;

    inline constexpr std::uint8_t TSS_ACCESS   = 0x89;

    inline constexpr uint8_t ACCESS_PRESENT    = 0x80;  // 1 << 7
    inline constexpr uint8_t ACCESS_RING_0     = 0x00;  // DPL = 0
    inline constexpr uint8_t ACCESS_RING_3     = 0x60;  // DPL = 3
    inline constexpr uint8_t ACCESS_CODE_DATA  = 0x10;  // S bit
    inline constexpr uint8_t ACCESS_EXECUTABLE = 0x08;  // Code segment
    inline constexpr uint8_t ACCESS_READABLE   = 0x02;  // Code can be read
    inline constexpr uint8_t ACCESS_WRITABLE   = 0x02;  // Data can be written
    inline constexpr std::uint8_t ACCESS_TSS   = 0x09;

    inline constexpr uint8_t KERNEL_CODE = ACCESS_PRESENT | ACCESS_RING_0 |
                                           ACCESS_CODE_DATA | ACCESS_EXECUTABLE | ACCESS_READABLE;

    inline constexpr uint8_t KERNEL_DATA = ACCESS_PRESENT | ACCESS_RING_0 |
                                           ACCESS_CODE_DATA | ACCESS_WRITABLE;

    inline constexpr uint8_t USER_CODE   = ACCESS_PRESENT | ACCESS_RING_3 |
                                           ACCESS_CODE_DATA | ACCESS_EXECUTABLE | ACCESS_READABLE;

    inline constexpr uint8_t USER_DATA   = ACCESS_PRESENT | ACCESS_RING_3 |
                                           ACCESS_CODE_DATA | ACCESS_WRITABLE;

    
    struct [[gnu::packed]] GdtEntry {
        std::uint16_t limit_low;
        std::uint16_t base_low;
        std::uint8_t base_mid;
        std::uint8_t access;
        std::uint8_t limit_high : 4;
        std::uint8_t flags      : 4;
        std::uint8_t base_high;
    };

    static_assert(sizeof(GdtEntry) == 8, "GDT entry must be 8 bytes");

    struct [[gnu::packed]] Gdtr {
        std::uint16_t limit;
        std::uint64_t base;
    };

    struct [[gnu::packed]] TssDescriptor {
        std::uint16_t limit_low;
        std::uint16_t base_low;
        std::uint8_t base_mid;
        std::uint8_t access;      // 0x89 = present, TSS available
        std::uint8_t limit_flags; // limit[16:19] | flags
        std::uint8_t base_high;
        std::uint32_t base_upper; // bits 32-63
        std::uint32_t reserved;   // must be 0
    };

    static_assert(sizeof(TssDescriptor) == 16, "TSS Descriptor must be 16 bytes");

    struct [[gnu::packed]] TssEntry {
        std::uint32_t reserved0;   // Unused
        std::uint64_t rsp0;        // Kernel stack pointer
        std::uint64_t rsp1;        // Ring 1 stack (unused)
        std::uint64_t rsp2;        // Ring 3 stack (unsed)
        std::uint64_t reserved1;   // Unused
        std::uint64_t ist1;        // Iterrupt stack table entries
        std::uint64_t ist2;
        std::uint64_t ist3;
        std::uint64_t ist4;
        std::uint64_t ist5;
        std::uint64_t ist6;
        std::uint64_t ist7;
        std::uint64_t reserved2;   // unused
        std::uint16_t reserved3;   // unused
        std::uint16_t iopb_offset; // sizeof(TssEntry)
    };

    static_assert(sizeof(TssEntry) == 104, "TSS Entry must be 104 bytes");

    struct [[gnu::packed]] GdtTable {
        GdtEntry zero;        // GDT[0]   offset = 0x00
        GdtEntry kernel_code; // GDT[1]   offset = 0x08
        GdtEntry kernel_data; // GDT[2]   offset = 0x10
        GdtEntry user_data;   // GDT[3]   offset = 0x18
        GdtEntry user_code;   // GDT[4]   offset = 0x20
        TssDescriptor tss;    // GDT[5,6] offset = 0x28-0x30
    };

    void init();
}
