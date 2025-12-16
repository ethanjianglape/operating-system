#include <kernel/console/console.hpp>
#include <kernel/console/font8x16.hpp>

namespace kernel::console {
    static driver_config null_driver = {
        .putchar_fn = nullptr,
        .get_color_fn = nullptr,
        .set_color_fn = nullptr,
        .clear_fn = nullptr,
        .put_pixel_fn = nullptr,
        .get_screen_width_fn = nullptr,
        .get_screen_height_fn = nullptr,
    };

    static std::uint32_t cursor_x = 0;
    static std::uint32_t cursor_y = 0;

    static std::uint32_t current_fg = static_cast<std::uint32_t>(RgbColor::WHITE);
    static std::uint32_t current_bg = static_cast<std::uint32_t>(RgbColor::BLACK);
    
    static driver_config* driver = &null_driver;

    void init(driver_config* config) {
        driver = config;
        cursor_x = 0;
        cursor_y = 0;
    }

    void newline(){
        cursor_x = 0;
        cursor_y += fonts::FONT_HEIGHT;
    }

    void putchar(char c) {
        if (driver->put_pixel_fn == nullptr) {
            return;
        }

        if (c == '\n') {
            newline();
            return;
        }

        const std::uint8_t* glyph = fonts::get_glyph(c);

        auto pixel_x = cursor_x;
        auto pixel_y = cursor_y;

        for (std::uint8_t gi = 0; gi < fonts::FONT_HEIGHT; gi++) {
            const std::uint8_t byte = glyph[gi];

            for (std::uint8_t pi = 0; pi < fonts::FONT_WIDTH; pi++) {
                const std::uint8_t pixel = (byte >> (fonts::FONT_WIDTH - pi - 1)) & 1;

                if (pixel == 1) {
                    driver->put_pixel_fn(pixel_x + pi, pixel_y + gi, current_fg);
                } else {
                    driver->put_pixel_fn(pixel_x + pi, pixel_y + gi, current_bg);
                }
            }
        }

        cursor_x += fonts::FONT_WIDTH;

        if (cursor_x >= driver->get_screen_width_fn()) {
            newline();
        }
    }

    void puts(const char* str, std::size_t length) {
        for (std::size_t i = 0; i < length; i++) {
            putchar(str[i]);
        }
    }

    void set_color(RgbColor fg, RgbColor bg) {
        current_fg = static_cast<std::uint32_t>(fg);
        current_bg = static_cast<std::uint32_t>(bg);
    }

    std::uint32_t get_current_fg() {
        return current_fg;
    }

    std::uint32_t get_current_bg() {
        return current_bg;
    }

    std::uint8_t get_color() {
        if (driver->get_color_fn != nullptr) {
            return driver->get_color_fn();
        }

        return 0x0F; // Default: white on black
    }

    void clear() {
        if (driver->clear_fn != nullptr) {
            driver->clear_fn();
            cursor_x = 0;
            cursor_y = 0;
        }
    }
}
