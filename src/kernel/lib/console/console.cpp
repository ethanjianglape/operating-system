#include <kernel/console/console.hpp>
#include <kernel/console/font8x16.hpp>

namespace kernel::console {
    // simulates writing to /dev/null, calls to kprintf() and kernel::log() will do nothing
    static ConsoleDriver null_driver = {
        .putchar = nullptr,
        .clear = nullptr,
        .put_pixel = nullptr,
        .get_pixel = nullptr,
        .get_screen_width = nullptr,
        .get_screen_height = nullptr,
        .mode = ConsoleMode::TEXT
    };

    static std::uint32_t cursor_x = 0;
    static std::uint32_t cursor_y = 0;

    static std::uint32_t current_fg = static_cast<std::uint32_t>(RgbColor::WHITE);
    static std::uint32_t current_bg = static_cast<std::uint32_t>(RgbColor::BLACK);

    static ConsoleDriver* driver = &null_driver;

    void init(ConsoleDriver* d) {
        driver = d;
        cursor_x = 0;
        cursor_y = 0;
    }

    void scroll() {
        if (driver->mode == ConsoleMode::SERIAL) {
            return;
        }
        
        const auto width = driver->get_screen_width();
        const auto height = driver->get_screen_height();
        
        for (std::uint32_t pixel_y = 0; pixel_y < height - fonts::FONT_HEIGHT; pixel_y++) {
            for (std::uint32_t pixel_x = 0; pixel_x < width; pixel_x++) {
                const auto pixel = driver->get_pixel(pixel_x, pixel_y + fonts::FONT_HEIGHT);
                driver->put_pixel(pixel_x, pixel_y, pixel);
            }
        }

        cursor_y -= fonts::FONT_HEIGHT;
    }

    void newline() {
        cursor_x = 0;
        cursor_y++;

        if (cursor_y + fonts::FONT_HEIGHT > driver->get_screen_height()) {
            scroll();
        }
    }

    void putchar(char c) {
        if (driver->putchar != nullptr) {
            driver->putchar(c);
            return;
        }
        
        if (driver->put_pixel == nullptr) {
            return;
        }

        if (c == '\n') {
            newline();
            return;
        }

        const std::uint8_t* glyph = fonts::get_glyph(c);

        const std::uint32_t pixel_x = cursor_x * fonts::FONT_WIDTH;
        const std::uint32_t pixel_y = cursor_y * fonts::FONT_HEIGHT;

        for (std::uint8_t y = 0; y < fonts::FONT_HEIGHT; y++) {
            const std::uint8_t byte = glyph[y];

            for (std::uint8_t x = 0; x < fonts::FONT_WIDTH; x++) {
                const std::uint8_t pixel = (byte >> (fonts::FONT_WIDTH - x - 1)) & 1;

                if (pixel == 1) {
                    driver->put_pixel(pixel_x + x, pixel_y + y, current_fg);
                } else {
                    driver->put_pixel(pixel_x + x, pixel_y + y, current_bg);
                }
            }
        }

        cursor_x++;

        if (cursor_x >= driver->get_screen_width()) {
            newline();
        }
    }

    void puts(const char* str, std::size_t length) {
        for (std::size_t i = 0; i < length; i++) {
            putchar(str[i]);
        }
    }

    void set_color(std::uint32_t fg, std::uint32_t bg) {
        current_fg = fg;
        current_bg = bg;
    }

    void set_color(RgbColor fg, RgbColor bg) {
        set_color(static_cast<std::uint32_t>(fg), static_cast<std::uint32_t>(bg));
    }

    std::uint32_t get_current_fg() {
        return current_fg;
    }

    std::uint32_t get_current_bg() {
        return current_bg;
    }

    void clear() {
        if (driver->clear != nullptr) {
            driver->clear();
            cursor_x = 0;
            cursor_y = 0;
        }
    }
}
