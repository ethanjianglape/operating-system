#pragma once

#include <kernel/console/console.hpp>

#include <cstdint>

namespace kernel::drivers::framebuffer {
    constexpr std::uint32_t RGB_BLACK = 0x00000000;
    constexpr std::uint32_t RGB_RED   = 0x00FF0000;
    constexpr std::uint32_t RGB_GREEN = 0x0000FF00;
    constexpr std::uint32_t RGB_BLUE  = 0x000000FF;
    
    struct FrameBufferInfo {
        std::uint64_t width;
        std::uint64_t height;
        std::uint64_t pitch;
        std::uint16_t bpp;
        std::uint8_t* vram;
    };

    void init(const FrameBufferInfo& info);

    kernel::console::ConsoleDriver* get_console_driver();

    void put_pixel(std::uint32_t x, std::uint32_t y, std::uint32_t color);
    std::uint32_t get_pixel(std::uint32_t x, std::uint32_t y);

    void clear_black();
    void clear(std::uint32_t color);

    void log();
}
