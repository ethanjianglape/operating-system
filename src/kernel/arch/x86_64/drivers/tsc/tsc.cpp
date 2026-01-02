#include "tsc.hpp"
#include "log/log.hpp"
#include <cstdint>
#include <cpuid.h>

namespace x86_64::drivers::tsc {
    std::uint64_t rdtsc() {
        std::uint64_t rax;
        std::uint64_t rdx;

        asm volatile("rdtsc" : "=a"(rax), "=d"(rdx) : :);

        std::uint64_t tsc = (rdx << 32) | rax;

        return tsc;
    }
}
