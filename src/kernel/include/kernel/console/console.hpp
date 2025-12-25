#pragma once

#include "containers/kstring.hpp"
#include <cstdint>

namespace kernel::console {
    enum class RgbColor : std::uint32_t {
        BLACK   = 0x00000000,
        WHITE   = 0x00FFFFFF,
        RED     = 0x00FF0000,
        GREEN   = 0x0000FF00,
        BLUE    = 0x000000FF,
        CYAN    = 0x0000FFFF,
        YELLOW  = 0x00FFFF00,
        MAGENTA = 0x00FF00FF,
    };

    void init();

    void set_cursor(std::uint32_t x, std::uint32_t y);
    void set_cursor_x(std::uint32_t x);
    void set_cursor_y(std::uint32_t y);
    void move_cursor(std::int32_t dx, std::int32_t dy);

    void draw_cursor();
    void clear_cursor();

    void clear_to_eol();

    void enable_cursor();
    void disable_cursor();

    void show_cursor();
    void hide_cursor();
    void toggle_cursor();

    void scroll();
    void newline();
    
    void set_color(std::uint32_t fg, std::uint32_t bg);
    void set_color(RgbColor fg, RgbColor bg);
    void reset_color();

    std::uint32_t get_current_fg();
    std::uint32_t get_current_bg();

    void clear();

    void backspace();

    int put(char c);
    int put(const char* str);
    int put(const unsigned char* str);
    int put(const kernel::kstring& str);
}
