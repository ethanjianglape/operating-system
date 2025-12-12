#pragma once

#include <cstdint>

namespace i386::process {
    struct [[gnu::packed]] iret_frame {
        std::uint32_t eip;
        std::uint32_t cs;
        std::uint32_t eflags;
        std::uint32_t esp;
        std::uint32_t ss;
    };
    
    void init();
}
