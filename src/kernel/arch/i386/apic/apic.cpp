#include "apic.hpp"

#include "arch/i386/cpu/cpu.hpp"
#include "arch/i386/interrupts/irq.hpp"
#include "arch/i386/timers/pit.hpp"
#include "arch/i386/timers/timer.hpp"
#include "kernel/tty.h"
#include <stdio.h>

using namespace i386;

// Read from Local APIC register
static inline std::uint32_t lapic_read(std::uint32_t reg) {
    return *((volatile uint32_t*)(apic::LAPIC_BASE_ADDR + reg));
}

// Write to Local APIC register
static inline void lapic_write(std::uint32_t reg, std::uint32_t value) {
    *((volatile uint32_t*)(apic::LAPIC_BASE_ADDR + reg)) = value;
}

// Check if APIC is supported
bool apic::check_support() {
    std::uint32_t eax;
    std::uint32_t edx;
    
    cpu::cpuid(1, &eax, &edx);

    // Check bit 9 of EDX (APIC feature flag)
    return (edx & CPUID_FEAT_EDX_APIC) != 0;
}

// Enable Local APIC via MSR
void apic::enable() {
    // Read current APIC base MSR
    std::uint64_t apic_base = cpu::rdmsr(MSR_APIC_BASE);

    // Set enable bit (bit 11)
    apic_base |= MSR_APIC_BASE_ENABLE;

    // Write back
    cpu::wrmsr(MSR_APIC_BASE, apic_base);
}

// Initialize Local APIC
void apic::init() {
    // Enable Local APIC via MSR
    apic::enable();

    // Set Spurious Interrupt Vector Register
    // Bit 8: Enable APIC
    // Bits 0-7: Spurious vector number (we'll use 0xFF)
    lapic_write(LAPIC_SPURIOUS, 0x1FF);  // Enable APIC + vector 0xFF

    // Clear task priority to enable all interrupts
    lapic_write(LAPIC_TPR, 0);
}

void apic::timer_init(const std::uint32_t vector, const std::uint32_t frequency) {
    constexpr std::uint32_t initial_count = 0xFFFFFFFF;
    
    lapic_write(LAPIC_TIMER_DIVIDE, TIMER_DIV_BY_16);
    lapic_write(LAPIC_TIMER_INIT_COUNT, initial_count);
    
    i386::pit::sleep_ms(10);

    lapic_write(LAPIC_TIMER, APIC_LVT_INT_MASKED);

    const std::uint32_t ticks = initial_count - lapic_read(LAPIC_TIMER_CURRENT);

    printf("APIC timer ticks: ");
    terminal_putint(ticks);
    printf("\n");

    lapic_write(LAPIC_TIMER, 32 | TIMER_MODE_PERIODIC);
    lapic_write(LAPIC_TIMER_DIVIDE, TIMER_DIV_BY_16);
    lapic_write(LAPIC_TIMER_INIT_COUNT, ticks);

    irq::register_irq_handler(32, timer::timer_tick);
}

// Send End of Interrupt signal
void apic::send_eoi() {
    lapic_write(LAPIC_EOI, 0);
}
