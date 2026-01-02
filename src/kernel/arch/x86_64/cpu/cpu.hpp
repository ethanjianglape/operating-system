#pragma once

#include <cstdint>
#include <cpuid.h>

// Forward declaration to avoid circular include
namespace x86_64::irq {
    struct InterruptFrame;
}

namespace x86_64::cpu {

    // =========================================================================
    // Debug Functions
    // =========================================================================

    // Dump current CPU state (control regs, segment regs, flags)
    void dump();

    // Dump CPU state from an interrupt context (includes all GPRs, RIP, etc.)
    void dump(const irq::InterruptFrame* frame);

    // =========================================================================
    // I/O Port Functions
    // =========================================================================

    [[gnu::always_inline]]
    inline void outb(const std::uint16_t port, const std::uint8_t value) {
        asm volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
    }

    [[gnu::always_inline]]
    inline std::uint8_t inb(const std::uint16_t port) {
        std::uint8_t ret;

        asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");

        return ret;
    }

    [[gnu::always_inline]]
    inline void cpuid(std::uint32_t code, std::uint32_t* a, std::uint32_t* d) {
        asm volatile("cpuid" : "=a"(*a), "=d"(*d) : "a"(code) : "ebx", "ecx", "memory");
    }

    inline void cpuid(std::uint32_t code, std::uint32_t* eax, std::uint32_t* ebx, std::uint32_t* ecx) {
        asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx) : "a"(code) : "edx", "memory");
    }

    [[gnu::always_inline]]
    inline std::uint64_t rdmsr(std::uint32_t msr) {
        std::uint32_t low;
        std::uint32_t high;

        asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr) : "memory");

        return ((uint64_t)high << 32) | low;
    }

    [[gnu::always_inline]]
    inline void wrmsr(uint32_t msr, uint64_t value) {
        const std::uint32_t low = value & 0xFFFFFFFF;
        const std::uint32_t high = value >> 32;

        asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr) : "memory");
    }

    [[gnu::always_inline]]
    inline void hlt() {
        asm volatile("hlt");
    }

    [[gnu::always_inline]]
    inline void pause() {
        asm volatile("pause");
    }

    [[gnu::always_inline]]
    inline void sti() {
        asm volatile("sti" : : : "memory");
    }

    [[gnu::always_inline]]
    inline void cli() {
        asm volatile("cli" : : : "memory");
    }
}

