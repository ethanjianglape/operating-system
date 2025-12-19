#pragma once

#include <cstdint>

namespace x86_64::idt {
    struct [[gnu::packed]] IdtEntry {
        std::uint16_t offset_low;
        std::uint16_t selector;
        std::uint8_t ist;
        std::uint8_t attributes;
        std::uint16_t offset_mid;
        std::uint32_t offset_high;
        std::uint32_t reserved;
    };

    static_assert(sizeof(IdtEntry) == 16, "IDT entries must be 16 bytes");

    struct [[gnu::packed]] Idtr {
        std::uint16_t limit;
        std::uint64_t base;
    };

    static_assert(sizeof(Idtr) == 10, "IDTR must be 10 bytes");

    void init();
}

