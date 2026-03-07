#pragma once

#include <cstdint>
#include <linux/mman.hpp>

namespace syscall {
    std::uintptr_t sys_brk(void* addr);
    std::uintptr_t sys_mmap(void* addr, std::size_t length, int prot, int flags, int fd, std::size_t offset);
    int sys_munmap(void* addr, std::size_t length);
}
