#pragma once

/**
 * @file mman.hpp
 * @brief Linux mmap/mprotect constants.
 *
 * These values match Linux's asm-generic/mman-common.h.
 */

namespace linux {
    constexpr int PROT_NONE  = 0x0;
    constexpr int PROT_READ  = 0x1;
    constexpr int PROT_WRITE = 0x2;
    constexpr int PROT_EXEC  = 0x4;

    constexpr int MAP_PRIVATE   = 0x02;
    constexpr int MAP_ANONYMOUS = 0x20;
}
