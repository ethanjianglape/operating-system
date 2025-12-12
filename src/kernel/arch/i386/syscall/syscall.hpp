#pragma once

#include <cstdint>

namespace i386::syscall {
    inline constexpr std::uint32_t SYSCALL_IRQ_VECTOR = 0x80;

    inline constexpr std::uint32_t ENOSYS = -1;

    enum class syscall_number : std::uint32_t {
        SYS_WRITE = 1,
        TEST = 42
    };

    struct [[gnu::packed]] registers {
        std::uint32_t edi;
        std::uint32_t esi;
        std::uint32_t ebp;
        std::uint32_t esp;
        std::uint32_t ebx;
        std::uint32_t edx;
        std::uint32_t ecx;
        std::uint32_t eax;
    };

    void init();
}
