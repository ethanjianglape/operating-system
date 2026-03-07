#pragma once

/**
 * @file syscall.hpp
 * @brief Linux x86_64 syscall numbers.
 *
 * These numbers match the Linux kernel's syscall table for x86_64.
 * See: arch/x86/entry/syscalls/syscall_64.tbl in the Linux source.
 */

#include <cstdint>

namespace linux {
    constexpr std::uint64_t SYS_READ       = 0;
    constexpr std::uint64_t SYS_WRITE      = 1;
    constexpr std::uint64_t SYS_OPEN       = 2;
    constexpr std::uint64_t SYS_CLOSE      = 3;
    constexpr std::uint64_t SYS_STAT       = 4;
    constexpr std::uint64_t SYS_FSTAT      = 5;
    constexpr std::uint64_t SYS_LSEEK      = 8;
    constexpr std::uint64_t SYS_MMAP       = 9;
    constexpr std::uint64_t SYS_MUNMAP     = 11;
    constexpr std::uint64_t SYS_BRK        = 12;
    constexpr std::uint64_t SYS_IOCTL      = 16;
    constexpr std::uint64_t SYS_WRITEV     = 20;
    constexpr std::uint64_t SYS_NANOSLEEP  = 35;
    constexpr std::uint64_t SYS_GETPID     = 39;
    constexpr std::uint64_t SYS_EXIT       = 60;
    constexpr std::uint64_t SYS_GETCWD     = 79;
    constexpr std::uint64_t SYS_CHDIR      = 80;
    constexpr std::uint64_t SYS_FCHDIR     = 81;
    constexpr std::uint64_t SYS_ARCH_PRCTL = 158;
    constexpr std::uint64_t SYS_SET_TID_ADDR = 218;
    constexpr std::uint64_t SYS_EXIT_GROUP = 231;
}
