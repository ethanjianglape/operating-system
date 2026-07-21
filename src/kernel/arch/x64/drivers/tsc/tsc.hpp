#pragma once

#include <cstdint>

namespace x64::drivers::tsc {

void init();

// [[gnu::always_inline]]
// inline std::uint64_t rdtsc();

std::uint64_t get_ticks();
std::uint64_t get_time_ns();
std::uint64_t get_time_us();
std::uint64_t get_time_ms();

}
