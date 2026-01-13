#pragma once

#include <cstddef>
#include <cstdint>

namespace x86_64::irq {
    constexpr std::size_t NUM_IRQ_HANDLERS = 256;

    // =========================================================================
    // x86 CPU Exception Vectors (0x00 - 0x1F)
    // =========================================================================
    //
    // These are hardwired by the CPU - we don't choose them. When a CPU error
    // occurs, the CPU generates an interrupt with the corresponding vector.
    //
    // Exceptions marked with (E) push an error code onto the stack.

    constexpr std::uint8_t EXC_DIVIDE_ERROR         = 0x00;  // #DE - Division by zero
    constexpr std::uint8_t EXC_DEBUG                = 0x01;  // #DB - Debug exception
    constexpr std::uint8_t EXC_NMI                  = 0x02;  // NMI - Non-maskable interrupt
    constexpr std::uint8_t EXC_BREAKPOINT           = 0x03;  // #BP - INT3 instruction
    constexpr std::uint8_t EXC_OVERFLOW             = 0x04;  // #OF - INTO instruction
    constexpr std::uint8_t EXC_BOUND_RANGE          = 0x05;  // #BR - BOUND instruction
    constexpr std::uint8_t EXC_INVALID_OPCODE       = 0x06;  // #UD - Invalid instruction
    constexpr std::uint8_t EXC_DEVICE_NOT_AVAIL     = 0x07;  // #NM - FPU not available
    constexpr std::uint8_t EXC_DOUBLE_FAULT         = 0x08;  // #DF - Double fault (E)
    constexpr std::uint8_t EXC_COPROC_SEGMENT       = 0x09;  // Coprocessor segment overrun (reserved)
    constexpr std::uint8_t EXC_INVALID_TSS          = 0x0A;  // #TS - Invalid TSS (E)
    constexpr std::uint8_t EXC_SEGMENT_NOT_PRESENT  = 0x0B;  // #NP - Segment not present (E)
    constexpr std::uint8_t EXC_STACK_SEGMENT        = 0x0C;  // #SS - Stack segment fault (E)
    constexpr std::uint8_t EXC_GENERAL_PROTECTION   = 0x0D;  // #GP - General protection fault (E)
    constexpr std::uint8_t EXC_PAGE_FAULT           = 0x0E;  // #PF - Page fault (E)
    constexpr std::uint8_t EXC_RESERVED_0F          = 0x0F;  // Reserved
    constexpr std::uint8_t EXC_X87_FPU              = 0x10;  // #MF - x87 FPU error
    constexpr std::uint8_t EXC_ALIGNMENT_CHECK      = 0x11;  // #AC - Alignment check (E)
    constexpr std::uint8_t EXC_MACHINE_CHECK        = 0x12;  // #MC - Machine check
    constexpr std::uint8_t EXC_SIMD                 = 0x13;  // #XM - SIMD floating-point
    constexpr std::uint8_t EXC_VIRTUALIZATION       = 0x14;  // #VE - Virtualization exception
    constexpr std::uint8_t EXC_CONTROL_PROTECTION   = 0x15;  // #CP - Control protection (E)
    // 0x16 - 0x1B: Reserved
    constexpr std::uint8_t EXC_HYPERVISOR_INJECT    = 0x1C;  // #HV - Hypervisor injection
    constexpr std::uint8_t EXC_VMM_COMMUNICATION    = 0x1D;  // #VC - VMM communication (E)
    constexpr std::uint8_t EXC_SECURITY             = 0x1E;  // #SX - Security exception (E)
    // 0x1F: Reserved

    constexpr std::uint8_t EXC_MAX                  = 0x1F;  // Last exception vector

    // =========================================================================
    // Hardware IRQ Vectors (0x20+)
    // =========================================================================
    //
    // These start at 0x20 because we remap the PIC/APIC to avoid conflicts
    // with CPU exceptions. The actual IRQ number = vector - IRQ_BASE.

    constexpr std::uint8_t IRQ_BASE                 = 0x20;  // IRQ 0 maps to vector 0x20
    constexpr std::uint8_t IRQ_TIMER                = 0x20;  // IRQ 0 - PIT/APIC timer
    constexpr std::uint8_t IRQ_KEYBOARD             = 0x21;  // IRQ 1 - Keyboard
    constexpr std::uint8_t IRQ_CASCADE              = 0x22;  // IRQ 2 - Cascade (PIC2)
    constexpr std::uint8_t IRQ_COM2                 = 0x23;  // IRQ 3 - COM2/COM4
    constexpr std::uint8_t IRQ_COM1                 = 0x24;  // IRQ 4 - COM1/COM3
    constexpr std::uint8_t IRQ_LPT2                 = 0x25;  // IRQ 5 - LPT2
    constexpr std::uint8_t IRQ_FLOPPY               = 0x26;  // IRQ 6 - Floppy disk
    constexpr std::uint8_t IRQ_LPT1                 = 0x27;  // IRQ 7 - LPT1 / Spurious
    constexpr std::uint8_t IRQ_RTC                  = 0x28;  // IRQ 8 - Real-time clock
    constexpr std::uint8_t IRQ_ACPI                 = 0x29;  // IRQ 9 - ACPI
    constexpr std::uint8_t IRQ_OPEN1                = 0x2A;  // IRQ 10 - Open
    constexpr std::uint8_t IRQ_OPEN2                = 0x2B;  // IRQ 11 - Open
    constexpr std::uint8_t IRQ_MOUSE                = 0x2C;  // IRQ 12 - PS/2 Mouse
    constexpr std::uint8_t IRQ_COPROC               = 0x2D;  // IRQ 13 - Coprocessor
    constexpr std::uint8_t IRQ_PRIMARY_ATA          = 0x2E;  // IRQ 14 - Primary ATA
    constexpr std::uint8_t IRQ_SECONDARY_ATA        = 0x2F;  // IRQ 15 - Secondary ATA

    struct [[gnu::packed]] InterruptFrame {
        // Pushed by isr_common (reverse order)
        std::uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        std::uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
        // Pushed by isr stub
        std::uint64_t vector;
        std::uint64_t err;
        // Pushed by CPU
        std::uint64_t rip, cs, rflags, rsp, ss;
    };

    static_assert(sizeof(InterruptFrame) == 176, "InterruptFrame must match isr.s stack layout");

    using irq_handler_fn = void (*)(InterruptFrame* frame);

    void register_irq_handler(const std::uint32_t vector, irq_handler_fn handler);
}

