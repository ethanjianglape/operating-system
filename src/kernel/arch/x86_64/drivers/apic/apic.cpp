/**
 * Advanced Programmable Interrupt Controller (APIC)
 * ==================================================
 *
 * What is the APIC?
 *
 *   The APIC is the modern interrupt controller for x86 systems, replacing the
 *   legacy 8259 PIC (which we disable in pic.cpp). It actually consists of two
 *   distinct components that work together:
 *
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │                              System Overview                            │
 *   └─────────────────────────────────────────────────────────────────────────┘
 *
 *   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
 *   │   Keyboard   │     │    Timer     │     │   Network    │
 *   │   (IRQ 1)    │     │   (IRQ 0)    │     │   (IRQ 11)   │
 *   └──────┬───────┘     └──────┬───────┘     └──────┬───────┘
 *          │                    │                    │
 *          ▼                    ▼                    ▼
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │                          I/O APIC                                       │
 *   │                  (Usually at 0xFEC00000)                                │
 *   │                                                                         │
 *   │  Routes external device interrupts to the appropriate CPU core.         │
 *   │  Contains a Redirection Table: IRQ → which CPU + which vector           │
 *   └───────────────────────────────┬─────────────────────────────────────────┘
 *                                   │
 *                                   │ (Interrupt messages via system bus)
 *                                   │
 *          ┌────────────────────────┼────────────────────────┐
 *          │                        │                        │
 *          ▼                        ▼                        ▼
 *   ┌─────────────┐          ┌─────────────┐          ┌─────────────┐
 *   │ Local APIC  │          │ Local APIC  │          │ Local APIC  │
 *   │   (CPU 0)   │          │   (CPU 1)   │          │   (CPU 2)   │
 *   │             │          │             │          │             │
 *   │ @ 0xFEE00000│          │ @ 0xFEE00000│          │ @ 0xFEE00000│
 *   └──────┬──────┘          └──────┬──────┘          └──────┬──────┘
 *          │                        │                        │
 *          ▼                        ▼                        ▼
 *   ┌─────────────┐          ┌─────────────┐          ┌─────────────┐
 *   │   CPU 0     │          │   CPU 1     │          │   CPU 2     │
 *   └─────────────┘          └─────────────┘          └─────────────┘
 *
 * Local APIC (LAPIC):
 *
 *   Each CPU core has its own Local APIC, memory-mapped at 0xFEE00000 (same
 *   virtual address, but each core sees its own LAPIC). The LAPIC handles:
 *
 *   - Receiving interrupts from the I/O APIC
 *   - Local timer (we use this for scheduling)
 *   - Inter-Processor Interrupts (IPIs) for multi-core communication
 *   - Priority arbitration (which interrupt to handle first)
 *   - End-of-Interrupt signaling (EOI)
 *
 * I/O APIC:
 *
 *   The I/O APIC sits on the motherboard and routes external device interrupts
 *   to the appropriate CPU. It has a Redirection Table that maps each IRQ to:
 *
 *   - A destination CPU (or set of CPUs)
 *   - An interrupt vector number
 *   - Trigger mode (edge/level)
 *   - Polarity (active high/low)
 *   - Delivery mode (fixed, lowest priority, etc.)
 *
 * Why APIC instead of PIC?
 *
 *   1. Multi-core support: PIC can only signal one CPU; APIC routes to any core
 *   2. More interrupts: 224 vectors vs PIC's 15
 *   3. MSI support: Modern PCIe devices use Message Signaled Interrupts
 *   4. Better priority: 16 priority levels vs PIC's fixed priority
 *   5. Performance: No need to mask/unmask, just send EOI
 *
 * Timer Calibration:
 *
 *   The LAPIC timer runs at the CPU's bus frequency, which varies by system.
 *   We calibrate it by:
 *   1. Set a known count and start it
 *   2. Wait a known time (using PIT, which has a fixed frequency)
 *   3. Read how many ticks elapsed → now we know ticks/ms
 *   4. Configure periodic interrupts at our desired rate
 */

#include "apic.hpp"
#include <timer/timer.hpp>

#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/interrupts/irq.hpp>
#include <arch/x86_64/memory/vmm.hpp>
#include <log/log.hpp>

#include <arch/x86_64/drivers/pit/pit.hpp>
#include <panic/panic.hpp>
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
        // Route keyboard (IRQ 1) to the specified interrupt vector.
        // Assumptions (valid for most systems):
        // - GSI base = 0, so IRQ 1 = I/O APIC pin 1
        // - Edge triggered, active high (default for keyboard)
        // - Destination = CPU 0 (bits 56-63, which default to 0)

        constexpr std::uint32_t KEYBOARD_IRQ = 1;
        const std::uint64_t entry = vector;  // Low 8 bits = vector, rest = 0 (defaults)

        ioapic_write(IOAPIC_REDTBL_LO(KEYBOARD_IRQ), static_cast<std::uint32_t>(entry));
        ioapic_write(IOAPIC_REDTBL_HI(KEYBOARD_IRQ), static_cast<std::uint32_t>(entry >> 32));
    }

    void init() {
        log::init_start("APIC");

        if (!check_support()) {
            panic("APIC not supported - required for this kernel");
        }

        if (ioapic_virt_base == nullptr || lapic_virt_base == nullptr) {
            panic("IOAPIC/LAPIC physical addresses have not been mapped yet!");
        }

        enable();

        // Set Spurious Interrupt Vector Register
        // Bit 8: Enable APIC
        // Bits 0-7: Spurious vector number (we'll use 0xFF)
        lapic_write(LAPIC_SPURIOUS, 0x1FF); // Enable APIC + vector 0xFF

        // Clear task priority to enable all interrupts
        lapic_write(LAPIC_TPR, 0);

        ioapic_route_keyboard(irq::IRQ_KEYBOARD);

        timer_init();

        log::init_end("APIC");
    }

    void apic_timer_handler(std::uint32_t vector) {
        timer::tick();
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

        lapic_write(LAPIC_TIMER, irq::IRQ_TIMER | TIMER_MODE_PERIODIC);
        lapic_write(LAPIC_TIMER_DIVIDE, TIMER_DIV_BY_16);
        lapic_write(LAPIC_TIMER_INIT_COUNT, ticks);

        irq::register_irq_handler(irq::IRQ_TIMER, apic_timer_handler);

        log::info("APIC timer initialized on vector ", irq::IRQ_TIMER, " at ", ms, "ms resolution.");
    }
}


