#pragma once

#include <cstdint>

namespace x86_64::tls {
    constexpr std::uint32_t MSR_FS_BASE = 0xC0000100;
    constexpr std::uint32_t ARCH_SET_FS = 0x1002;

    void set_fs_base(std::uintptr_t addr);
}
