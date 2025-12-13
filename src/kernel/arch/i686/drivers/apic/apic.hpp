#pragma once

#include <cstdint>

namespace i686::drivers::apic {
    // Local APIC registers (offset from base address 0xFEE00000)
    inline constexpr std::uint32_t LAPIC_ID                = 0x0020;  // Local APIC ID
    inline constexpr std::uint32_t LAPIC_VERSION           = 0x0030;  // Local APIC Version
    inline constexpr std::uint32_t LAPIC_TPR               = 0x0080;  // Task Priority Register
    inline constexpr std::uint32_t LAPIC_EOI               = 0x00B0;  // End of Interrupt
    inline constexpr std::uint32_t LAPIC_SPURIOUS          = 0x00F0;  // Spurious Interrupt Vector Register
    inline constexpr std::uint32_t LAPIC_ESR               = 0x0280;  // Error Status Register
    inline constexpr std::uint32_t LAPIC_ICR_LOW           = 0x0300;  // Interrupt Command Register (low)
    inline constexpr std::uint32_t LAPIC_ICR_HIGH          = 0x0310;  // Interrupt Command Register (high)
    inline constexpr std::uint32_t LAPIC_TIMER             = 0x0320;  // LVT Timer Register
    inline constexpr std::uint32_t LAPIC_TIMER_INIT_COUNT  = 0x0380;  // Timer Initial Count
    inline constexpr std::uint32_t LAPIC_TIMER_CURRENT     = 0x0390;  // Timer Current Count
    inline constexpr std::uint32_t LAPIC_TIMER_DIVIDE      = 0x03E0;  // Timer Divide Configuration
    inline constexpr std::uint32_t APIC_LVT_INT_MASKED     = 0x10000;

    inline constexpr std::uint32_t TIMER_MODE_PERIODIC     = 0x20000;
    inline constexpr std::uint32_t TIMER_MODE_ONESHOT      = 0x00000;
    inline constexpr std::uint32_t TIMER_DIV_BY_16         = 0x3;

    // LAPIC base address (standard location)
    inline constexpr std::uint32_t LAPIC_BASE_ADDR         = 0xFEE00000;

    // Spurious interrupt vector register bits
    inline constexpr std::uint32_t LAPIC_SPURIOUS_ENABLE   = 0x100;   // Bit 8: APIC Software Enable/Disable

    // MSR for APIC base address
    inline constexpr std::uint32_t MSR_APIC_BASE           = 0x1B;
    inline constexpr std::uint32_t MSR_APIC_BASE_ENABLE    = 0x800;   // Bit 11: Enable Local APIC

    // CPUID feature flags
    inline constexpr std::uint32_t CPUID_FEAT_EDX_APIC     = (1 << 9);  // APIC available

    bool check_support();
    void enable();
    void init();
    void send_eoi();
    void timer_init();
}
