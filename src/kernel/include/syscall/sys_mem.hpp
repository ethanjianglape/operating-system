#pragma once

#include <cstdint>

namespace syscall {
    constexpr int PROT_READ  = 0x1;
    constexpr int PROT_WRITE = 0x2;
    constexpr int PROT_EXEC  = 0x4;
    constexpr int PROT_NONE  = 0x0;

    constexpr int MAP_PRIVATE   = 0x02;
    constexpr int MAP_ANONYMOUS = 0x20;
    
    std::uintptr_t sys_brk(void* addr);
    std::uintptr_t sys_mmap(void* addr, std::size_t length, int prot, int flags, int fd, std::size_t offset);
    int sys_munmap(void* addr, std::size_t length);
}
