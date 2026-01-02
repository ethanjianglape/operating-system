#pragma once

#include <cstdint>

namespace x86_64::drivers::tsc {
    std::uint64_t rdtsc();
    
    void test();
}
