#pragma once

#include <cstdint>

namespace x86_64::idt {
    struct [[gnu::packed]] idt_entry_t {
        std::uint16_t isr_low;
        std::uint16_t kernel_cs;
        std::uint8_t reserved;
        std::uint8_t attributes;
        std::uint16_t isr_high;
    };

    struct [[gnu::packed]] idtr_t {
        std::uint16_t limit;
        std::uint32_t base;
    };

    void init();
}

