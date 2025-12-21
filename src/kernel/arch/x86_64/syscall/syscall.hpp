#pragma once

#include <cstdint>

namespace x86_64::syscall {
    constexpr std::uint32_t SYSCALL_IRQ_VECTOR = 0x80;
    constexpr std::uint32_t SYSCALL_SYS_WRITE = 4;

    constexpr std::uint32_t MSR_EFER           = 0xC0000080;
    constexpr std::uint32_t MSR_STAR           = 0xC0000081;
    constexpr std::uint32_t MSR_LSTAR          = 0xC0000082;
    constexpr std::uint32_t MSR_SFMASK         = 0xC0000084;
    constexpr std::uint32_t MSR_GS_BASE        = 0xC0000101;
    constexpr std::uint32_t MSR_KERNEL_GS_BASE = 0xC0000102;

    constexpr std::uint32_t EFER_SCE = (1 << 0);

    constexpr std::uint64_t SFMASK_IF = (1 << 9);  // Interrupt Flag (disable interrupts on entry)
    constexpr std::uint64_t SFMASK_DF = (1 << 10); // Direction Flag (ensure string ops go forward)
    constexpr std::uint64_t SFMASK_TF = (1 << 8);  // Trap Flag (Disable single-stepping)

    constexpr std::uint32_t ENOSYS = -1;

    enum class syscall_number : std::uint32_t {
        SYS_WRITE = 4,
        TEST = 42
    };

    struct [[gnu::packed]] SyscallFrame {
        std::uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        std::uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    };

    struct [[gnu::packed]] PerCPU {
        std::uint64_t kernel_rsp;
        std::uint64_t user_rsp;
    };

    void init();
}
