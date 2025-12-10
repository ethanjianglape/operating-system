#pragma once

#include <cstdint>

namespace i386::timer {
    void timer_tick(std::uint32_t vector);
}
