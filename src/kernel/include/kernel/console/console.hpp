#pragma once

#include <cstdint>

namespace kernel::console {
    using putchar_fn = void (*)(char);
    using clear_fn = void (*)(void);
    using put_pixel_fn = void (*)(std::uint32_t x, std::uint32_t y, std::uint32_t color);
    using get_pixel_fn = std::uint32_t (*)(std::uint32_t x, std::uint32_t y);
    using get_screen_width_fn = std::uint32_t (*)(void);
    using get_screen_height_fn = std::uint32_t (*)(void);

    enum class ConsoleMode : std::uint32_t {
        TEXT = 1,
        SERIAL = 2,
        PIXEL_BUFFER = 3
    };

    struct ConsoleDriver {
        putchar_fn putchar = nullptr;
        clear_fn clear = nullptr;
        put_pixel_fn put_pixel = nullptr;
        get_pixel_fn get_pixel = nullptr;
        get_screen_width_fn get_screen_width = nullptr;
        get_screen_height_fn get_screen_height = nullptr;
        ConsoleMode mode;
    };

    enum class RgbColor : std::uint32_t {
        BLACK = 0x00000000,
        WHITE = 0x00FFFFFF,
        RED   = 0x00FF0000,
        GREEN = 0x0000FF00,
        BLUE  = 0x000000FF,
        CYAN  = 0x0000FFFF,
    };

    enum class color : std::uint8_t {
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
        LIGHT_BROWN = 14,
        WHITE = 15,
    };

    void init(ConsoleDriver* driver);

    void scroll();
    void newline();
    
    void putchar(char c);
    void puts(const char* str, std::size_t length);

    void set_color(std::uint32_t fg, std::uint32_t bg);
    void set_color(RgbColor fg, RgbColor bg);
    
    std::uint32_t get_current_fg();
    std::uint32_t get_current_bg();

    void clear();
}
