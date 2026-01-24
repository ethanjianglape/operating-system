#pragma once

#include <cstdint>

namespace syscall {
    std::uintptr_t sys_brk(void* addr);
    int sys_mmap();
}
