#pragma once

#include <cstdint>

namespace vga {

    constexpr int BUFFER_COLS = 80;
    constexpr int BUFFER_ROWS = 25;

    inline volatile auto* buffer = reinterpret_cast<std::uint16_t*>(0xB8000);

    enum class Color : std::uint8_t {
        BLACK = 0,
        BLUE = 1,
        GREEN = 2,
        CYAN = 3,
        RED = 4,
        MAGENTA = 5,
        BROWN = 6,
        LIGHT_GREY = 7,
        DARK_GREY = 8,
        LIGHT_BLUE = 9,
        LIGHT_GREEN = 10,
        LIGHT_CYAN = 11,
        LIGHT_RED = 12,
        LIGHT_MAGENTA = 13,
        YELLOW = 14,
        WHITE = 15,
    };

    void initialize();

    void clear_screen();

    void write_double_buffer();

    void scroll();

    void newline();

    void print_int(int n, const Color color = Color::WHITE);

    void print_char(const char c, const Color color = Color::WHITE);

    void print_str(const char* str, const Color color = Color::WHITE);

    void printf(const char* format, ...);

} // namespace vga
