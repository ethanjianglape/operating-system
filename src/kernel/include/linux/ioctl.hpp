#pragma once

/**
 * @file ioctl.hpp
 * @brief Linux ioctl constants and struct layouts.
 *
 * Terminal ioctl values match Linux's asm-generic/ioctls.h.
 * Struct layouts match Linux's include/uapi/asm-generic/termbits.h.
 */

#include <cstdint>

namespace linux {
    constexpr unsigned long TIOCGWINSZ = 0x5413;

    struct winsize {
        std::uint16_t ws_row;
        std::uint16_t ws_col;
        std::uint16_t ws_xpixel;
        std::uint16_t ws_ypixel;
    };

    struct iovec {
        void*       iov_base;
        std::size_t iov_len;
    };
}
