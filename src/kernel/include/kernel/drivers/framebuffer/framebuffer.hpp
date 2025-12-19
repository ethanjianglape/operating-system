#pragma once

#include <kernel/console/console.hpp>

#include <cstdint>

namespace kernel::drivers::framebuffer {
    inline constexpr std::uint32_t RGB_BLACK = 0x00000000;
    inline constexpr std::uint32_t RGB_RED   = 0x00FF0000;
    inline constexpr std::uint32_t RGB_GREEN = 0x0000FF00;
    inline constexpr std::uint32_t RGB_BLUE  = 0x000000FF;
    
    struct FrameBufferInfo {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t pitch;
        std::uint8_t bpp;
        void* physical_addr;
    };

    void config(const FrameBufferInfo& info);
    void init();

    kernel::console::ConsoleDriver* get_console_driver();

    void put_pixel(std::uint32_t x, std::uint32_t y, std::uint32_t color);
    std::uint32_t get_pixel(std::uint32_t x, std::uint32_t y);

    void draw_bitmap(std::uint32_t x,
                     std::uint32_t y,
                     std::uint32_t width,
                     std::uint32_t height,
                     std::uint8_t* bitmap,
                     std::uint32_t fg,
                     std::uint32_t bg);

    void clear_black();
    void clear(std::uint32_t color);

    void log();
}
