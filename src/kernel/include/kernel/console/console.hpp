#pragma once

#include <cstdint>

namespace kernel::console {
    using console_putchar_fn = void (*)(char);
    using console_set_color_fn = void (*)(std::uint8_t fg, std::uint8_t bg);
    using console_get_color_fn = std::uint8_t (*)(void);
    using console_clear_fn = void (*)(void);
    using console_put_pixel_fn = void (*)(std::uint32_t x, std::uint32_t y, std::uint32_t color);
    using console_get_screen_width_fn = std::uint32_t (*)(void);
    using console_get_screen_height_fn = std::uint32_t (*)(void);

    struct driver_config {
        console_putchar_fn putchar_fn = nullptr;
        console_get_color_fn get_color_fn = nullptr;
        console_set_color_fn set_color_fn = nullptr;
        console_clear_fn clear_fn = nullptr;
        console_put_pixel_fn put_pixel_fn = nullptr;
        console_get_screen_width_fn get_screen_width_fn = nullptr;
        console_get_screen_height_fn get_screen_height_fn = nullptr;
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

    void init(driver_config* config);

    void newline();
    void putchar(char c);
    void puts(const char* str, std::size_t length);

    void set_color(RgbColor fg, RgbColor bg);
    std::uint32_t get_current_fg();
    std::uint32_t get_current_bg();
    std::uint8_t get_color();

    void clear();
}
