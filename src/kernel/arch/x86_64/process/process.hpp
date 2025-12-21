#pragma once

#include <cstdint>

namespace x86_64::process {
    struct [[gnu::packed]] UserspaceFrame {
        std::uint64_t rip;
        std::uint64_t cs;
        std::uint64_t eflags;
        std::uint64_t rsp;
        std::uint64_t ss;
    };
    
    void init();
}
