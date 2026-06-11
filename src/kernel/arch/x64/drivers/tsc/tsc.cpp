#include "tsc.hpp"
#include "log/log.hpp"
#include <cpuid.h>
#include <cstdint>

namespace x64::drivers::tsc {
std::uint64_t rdtsc()
{
    std::uint64_t rax;
    std::uint64_t rdx;

    asm volatile("rdtsc" : "=a"(rax), "=d"(rdx) : :);

    std::uint64_t tsc = (rdx << 32) | rax;

    return tsc;
}
}
