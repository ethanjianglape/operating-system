#pragma once

#include <cstdint>

namespace syscall {
    constexpr int ARCH_SET_FS = 0x1002;
    constexpr int ARCH_GET_FS = 0x1003;

    int sys_arch_prctl(int code, std::uintptr_t addr);
}
