#pragma once

#include <cstdint>

namespace x86_64::timers {
    void timer_tick(std::uint32_t vector);
}
