#pragma once

#include <cstdint>

namespace x86_64::entry {
    constexpr std::uint32_t MSR_EFER   = 0xC0000080;
    constexpr std::uint32_t MSR_STAR   = 0xC0000081;
    constexpr std::uint32_t MSR_LSTAR  = 0xC0000082;
    constexpr std::uint32_t MSR_SFMASK = 0xC0000084;

    constexpr std::uint32_t EFER_SCE = (1 << 0);

    constexpr std::uint64_t SFMASK_IF = (1 << 9);  // Interrupt Flag (disable interrupts on entry)
    constexpr std::uint64_t SFMASK_DF = (1 << 10); // Direction Flag (ensure string ops go forward)
    constexpr std::uint64_t SFMASK_TF = (1 << 8);  // Trap Flag (Disable single-stepping)

    constexpr std::uint64_t SYS_READ     = 0;
    constexpr std::uint64_t SYS_WRITE    = 1;
    constexpr std::uint64_t SYS_LSEEK    = 8;
    constexpr std::uint64_t SYS_SLEEP_MS = 35;
    constexpr std::uint64_t SYS_GETPID   = 39;
    constexpr std::uint64_t SYS_EXIT     = 60;

    struct [[gnu::packed]] SyscallFrame {
        std::uint64_t ss, cs, r15, r14, r13, r12, r11, r10, r9, r8;
        std::uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    };

    void init();
}
