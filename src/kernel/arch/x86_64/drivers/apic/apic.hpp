#pragma once

#include <cstdint>

namespace x86_64::drivers::apic {
    // Local APIC registers (offset from base address 0xFEE00000)
    constexpr std::uint32_t LAPIC_ID                = 0x0020;  // Local APIC ID
    constexpr std::uint32_t LAPIC_VERSION           = 0x0030;  // Local APIC Version
    constexpr std::uint32_t LAPIC_TPR               = 0x0080;  // Task Priority Register
    constexpr std::uint32_t LAPIC_EOI               = 0x00B0;  // End of Interrupt
    constexpr std::uint32_t LAPIC_SPURIOUS          = 0x00F0;  // Spurious Interrupt Vector Register
    constexpr std::uint32_t LAPIC_ESR               = 0x0280;  // Error Status Register
    constexpr std::uint32_t LAPIC_ICR_LOW           = 0x0300;  // Interrupt Command Register (low)
    constexpr std::uint32_t LAPIC_ICR_HIGH          = 0x0310;  // Interrupt Command Register (high)
    constexpr std::uint32_t LAPIC_TIMER             = 0x0320;  // LVT Timer Register
    constexpr std::uint32_t LAPIC_TIMER_INIT_COUNT  = 0x0380;  // Timer Initial Count
    constexpr std::uint32_t LAPIC_TIMER_CURRENT     = 0x0390;  // Timer Current Count
    constexpr std::uint32_t LAPIC_TIMER_DIVIDE      = 0x03E0;  // Timer Divide Configuration
    constexpr std::uint32_t APIC_LVT_INT_MASKED     = 0x10000;
    
    constexpr std::uint32_t TIMER_MODE_PERIODIC     = 0x20000;
    constexpr std::uint32_t TIMER_MODE_ONESHOT      = 0x00000;
    constexpr std::uint32_t TIMER_DIV_BY_16         = 0x3;
    
    // LAPIC base address (standard location)
    constexpr std::uint32_t LAPIC_BASE_ADDR         = 0xFEE00000;
    
    // Spurious interrupt vector register bits
    constexpr std::uint32_t LAPIC_SPURIOUS_ENABLE   = 0x100;   // Bit 8: APIC Software Enable/Disable
    
    // MSR for APIC base address
    constexpr std::uint32_t MSR_APIC_BASE           = 0x1B;
    constexpr std::uint32_t MSR_APIC_BASE_ENABLE    = 0x800;   // Bit 11: Enable Local APIC
    
    // CPUID feature flags
    constexpr std::uint32_t CPUID_FEAT_EDX_APIC     = (1 << 9);  // APIC available
    
    // I/O APIC registers (indirect access via IOREGSEL/IOWIN)
    constexpr std::uintptr_t IOAPIC_IOREGSEL = 0x00;  // Register select (write reg number here)
    constexpr std::uintptr_t IOAPIC_IOWIN    = 0x10;  // Data window (read/write reg value here)
    
    // I/O APIC Redirection Table entries (each pin has 2 32-bit registers)
    // IOREDTBL[n] = base + 2*n, where n is the pin number (0-23)
    constexpr std::uint32_t IOAPIC_REDTBL_BASE = 0x10;  // First redirection entry

    volatile std::uint8_t* get_lapic_addr();
    
    bool check_support();
    void enable_apic();
    void init();
    void send_eoi();
    void timer_init();
    void ioapic_route_irq(std::uint8_t irq, std::uint8_t vector);
}
