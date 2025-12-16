#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>

namespace kernel {
    int kprintf(const char* str, ...);

    int kvprintf(const char* str, std::va_list args);

    namespace console {
        using console_putchar_fn = void (*)(char);
        using console_set_color_fn = void (*)(std::uint8_t fg, std::uint8_t bg);
        using console_get_color_fn = std::uint8_t (*)(void);
        using console_clear_fn = void (*)(void);

        struct driver_config {
            console_putchar_fn putchar_fn = nullptr;
            console_get_color_fn get_color_fn = nullptr;
            console_set_color_fn set_color_fn = nullptr;
            console_clear_fn clear_fn = nullptr;
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
        
        void putchar(char c);
        void puts(const char* str, std::size_t length);

        void set_color(color fg, color bg);
        std::uint8_t get_color();

        void clear();
    }
}
