#pragma once

#include <cstdint>

namespace x86_64::context {
    struct [[gnu::packed]] ContextFrame {
        std::uint64_t r15, r14, r13, r12;
        std::uint64_t rbx, rbp;
        std::uint64_t rip;
    };
}
