#include "apic.hpp"
#include "kernel/timer/timer.hpp"

#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/interrupts/irq.hpp>
#include <arch/x86_64/vmm/vmm.hpp>
#include <kernel/log/log.hpp>

#include <arch/x86_64/drivers/pit/pit.hpp>
#include <kernel/panic/panic.hpp>
#include <cstdint>

namespace x86_64::drivers::apic {
    static volatile std::uint8_t* lapic_virt_base = nullptr;
    static volatile std::uint8_t* ioapic_virt_base = nullptr;

    static inline std::uint32_t lapic_read(std::uint32_t reg) {
        return *reinterpret_cast<volatile std::uint32_t*>(lapic_virt_base + reg);
    }

    static inline void lapic_write(std::uint32_t reg, std::uint32_t value) {
        *reinterpret_cast<volatile std::uint32_t*>(lapic_virt_base + reg) = value;
    }

    static inline std::uint32_t ioapic_read(std::uint32_t reg) {
        *reinterpret_cast<volatile std::uint32_t*>(ioapic_virt_base + IOAPIC_IOREGSEL) = reg;
        return *reinterpret_cast<volatile std::uint32_t*>(ioapic_virt_base + IOAPIC_IOWIN);
    }

    static inline void ioapic_write(std::uint32_t reg, std::uint32_t value) {
        *reinterpret_cast<volatile std::uint32_t*>(ioapic_virt_base + IOAPIC_IOREGSEL) = reg;
        *reinterpret_cast<volatile std::uint32_t*>(ioapic_virt_base + IOAPIC_IOWIN) = value;
    }

    void set_lapic_addr(std::uintptr_t lapic_phys_addr) {
        lapic_virt_base =
            reinterpret_cast<volatile std::uint8_t*>(vmm::map_hddm_page(lapic_phys_addr, vmm::PAGE_CACHE_DISABLE));
    }

    void set_ioapic_addr(std::uintptr_t ioapic_phys_addr) {
        ioapic_virt_base =
            reinterpret_cast<volatile std::uint8_t*>(vmm::map_hddm_page(ioapic_phys_addr, vmm::PAGE_CACHE_DISABLE));
    }

    void send_eoi() {
        lapic_write(LAPIC_EOI, 0);
    }

    bool check_support() {
        std::uint32_t eax;
        std::uint32_t edx;

        cpu::cpuid(1, &eax, &edx);

        return (edx & CPUID_FEAT_EDX_APIC) != 0;
    }

    void enable() {
        std::uint64_t apic_msr = cpu::rdmsr(MSR_APIC_BASE);
        apic_msr |= MSR_APIC_BASE_ENABLE;
        cpu::wrmsr(MSR_APIC_BASE, apic_msr);
    }

    void ioapic_route_keyboard(std::uint8_t vector) {
        // For now, use these hardcoded assumptions:
        // - GSI base = 0, so IRQ1 = IOAPIC Pin 1
        // - No overrides, so edge triggered, active high
        
        std::uint64_t entry = 0;

        entry |= vector;
        entry |= 0UL << 56;

        ioapic_write(0x12, static_cast<std::uint32_t>(entry));
        ioapic_write(0x13, static_cast<std::uint32_t>(entry >> 32));
    }

    void init() {
        kernel::log::init_start("APIC");

        if (!check_support()) {
            kernel::panic("APIC not supported - required for this kernel");
        }

        if (ioapic_virt_base == nullptr || lapic_virt_base == nullptr) {
            kernel::panic("IOAPIC/LAPIC physical addresses have not been mapped yet!");
        }

        enable();

        // Set Spurious Interrupt Vector Register
        // Bit 8: Enable APIC
        // Bits 0-7: Spurious vector number (we'll use 0xFF)
        lapic_write(LAPIC_SPURIOUS, 0x1FF); // Enable APIC + vector 0xFF

        // Clear task priority to enable all interrupts
        lapic_write(LAPIC_TPR, 0);

        ioapic_route_keyboard(33);

        timer_init();

        kernel::log::init_end("APIC");
    }

    void apic_timer_handler(std::uint32_t vector) {
        kernel::timer::tick();
        send_eoi();
    }

    void timer_init() {
        constexpr std::uint32_t initial_count = 0xFFFFFFFF;
        constexpr std::uint32_t ms = 10;

        lapic_write(LAPIC_TIMER_DIVIDE, TIMER_DIV_BY_16);
        lapic_write(LAPIC_TIMER_INIT_COUNT, initial_count);

        pit::sleep_ms(ms);

        lapic_write(LAPIC_TIMER, APIC_LVT_INT_MASKED);

        const std::uint32_t ticks = initial_count - lapic_read(LAPIC_TIMER_CURRENT);

        lapic_write(LAPIC_TIMER, 32 | TIMER_MODE_PERIODIC);
        lapic_write(LAPIC_TIMER_DIVIDE, TIMER_DIV_BY_16);
        lapic_write(LAPIC_TIMER_INIT_COUNT, ticks);

        irq::register_irq_handler(32, apic_timer_handler);

        kernel::log::info("APIC timer initialized on ISR32 (IRQ0) at ", ms, "ms resoluation.");
    }
}


