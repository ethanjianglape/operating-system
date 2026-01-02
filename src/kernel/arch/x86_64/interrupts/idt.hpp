#pragma once

#include <cstddef>
#include <cstdint>

namespace x86_64::idt {
    constexpr std::size_t IDT_MAX_DESCRIPTORS = 256;

    // IDT attribute flags
    constexpr std::uint8_t IDT_PRESENT      = 0x80;  // Entry is valid
    constexpr std::uint8_t IDT_DPL_RING0    = 0x00;  // Kernel only
    constexpr std::uint8_t IDT_DPL_RING3    = 0x60;  // Userspace can trigger
    constexpr std::uint8_t IDT_INTERRUPT    = 0x0E;  // 64-bit interrupt gate

    // Combined attribute values
    constexpr std::uint8_t IDT_KERNEL_INT   = IDT_PRESENT | IDT_DPL_RING0 | IDT_INTERRUPT;  // 0x8E
    constexpr std::uint8_t IDT_USER_INT     = IDT_PRESENT | IDT_DPL_RING3 | IDT_INTERRUPT;  // 0xEE

    // Kernel code segment selector (must match GDT layout)
    constexpr std::uint16_t KERNEL_CODE_SEL = 0x08;

    constexpr std::size_t IDT_VECTOR_SYSCALL = 0x80;
    
    struct [[gnu::packed]] IdtEntry {
        std::uint16_t offset_low;  // ISR address low
        std::uint16_t selector;
        std::uint8_t  ist;
        std::uint8_t  attributes;
        std::uint16_t offset_mid;  // ISR address mid
        std::uint32_t offset_high; // ISR address high
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

