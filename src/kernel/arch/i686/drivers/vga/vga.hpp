#pragma once

#include <kernel/console/console.hpp>

#include <cstddef>
#include <cstdint>

namespace i686::drivers::vga {
    inline constexpr std::size_t VGA_WIDTH = 80;
    inline constexpr std::size_t VGA_HEIGHT = 25;

    inline constexpr std::size_t VGA_BUFFER_ADDR = 0xB8000;
    
    void init();

    kernel::console::driver_config* get_driver();

    void putchar(char c);

    void set_color(std::uint8_t fg, std::uint8_t bg);
    std::uint8_t get_color();

    void newline();
    void clear();
    void scroll();
}
