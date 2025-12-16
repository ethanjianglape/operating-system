#pragma once

#include <cstdint>

namespace kernel::boot {
    void init(std::uint32_t mb_magic, std::uint32_t mbi_addr);
}
