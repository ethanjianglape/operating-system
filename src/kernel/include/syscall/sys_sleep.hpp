#pragma once

#include <cstdint>

namespace syscall {
    int sys_sleep_ms(std::uint64_t ms);
}
