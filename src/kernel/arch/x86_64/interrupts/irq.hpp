#pragma once

#include <cstdint>

namespace x86_64::irq {
    using irq_handler_t = void (*)(std::uint32_t vector);

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

    void register_irq_handler(const std::uint32_t vector, irq_handler_t handler);
}

