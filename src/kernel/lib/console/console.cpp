#include <concepts>
#include <cstdint>
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

    static std::uint32_t prev_fg = current_fg;
    static std::uint32_t prev_bg = current_bg;

    static ConsoleDriver* driver = &null_driver;

    static NumberFormat prev_number_format = NumberFormat::DEC;

    static NumberFormat number_format = NumberFormat::DEC;

    constexpr char DEC_CHARS[] = "0123456789";

    constexpr char HEX_CHARS[] = "0123456789ABCDEF";

    constexpr char OCT_CHARS[] = "01234567";

    constexpr char BIN_CHARS[] = "01";

    void init(ConsoleDriver* d) {
        driver = d;
        cursor_x = 0;
        cursor_y = 0;
    }

    void set_number_format(NumberFormat f) {
        prev_number_format = number_format;
        number_format = f;
    }

    void reset_number_format() {
        number_format = prev_number_format;
    }

    NumberFormat get_number_format() {
        return number_format;
    }

    int number_format_divisor() {
        switch (number_format) {
        case NumberFormat::DEC:
            return 10;
        case NumberFormat::HEX:
            return 16;
        case NumberFormat::OCT:
            return 8;
        case NumberFormat::BIN:
            return 2;
        default:
            return 10;
        }
    }

    char number_format_char(int i) {
        switch (number_format) {
        case NumberFormat::DEC:
            return DEC_CHARS[i];
        case NumberFormat::HEX:
            return HEX_CHARS[i];
        case NumberFormat::OCT:
            return OCT_CHARS[i];
        case NumberFormat::BIN:
            return BIN_CHARS[i];
        default:
            return ' ';
        }
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

        cursor_y--;
    }

    void newline() {
        cursor_x = 0;
        cursor_y++;

        const auto py = (cursor_y + 1) * fonts::FONT_HEIGHT;

        if (py > driver->get_screen_height()) {
            scroll();
        }
    }

    int put(char c) {
        if (driver->putchar != nullptr) {
            driver->putchar(c);
            return 0;
        }
        
        if (driver->put_pixel == nullptr) {
            return 0;
        }

        if (c == '\n') {
            newline();
            return 1;
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

        const auto px = (cursor_x + 1) * fonts::FONT_WIDTH;

        if (px >= driver->get_screen_width()) {
            newline();
        }

        return 1;
    }

    int put(const char* str) {
        int written = 0;
        
        while (*str) {
            written += put(*str++);
        }

        return written;
    }

    int put(const unsigned char* str) {
        int written = 0;
        
        while (*str) {
            written += put(*str++);
        }

        return written;
    }

    int put(std::intmax_t num) {
        std::uintmax_t unum;
        int written = 0;

        if (num < 0) {
            written += put('-');
            unum = -static_cast<std::uintmax_t>(num);
        } else {
            unum = num;
        }

        return written + put(unum);
    }

    int put(std::uintmax_t unum) {
        const auto divisor = number_format_divisor();
        int written = 0;
        int i = 0;
        char buff[16];

        if (unum == 0) {
            written += put('0');
        } else {
            while (unum > 0) {
                const auto index = unum % divisor;
                const auto c = number_format_char(index);
                
                buff[i++] = c;
                unum /= divisor;
            }

            while (i > 0) {
                written += put(buff[--i]);
            }
        }

        return written;        
    }

    void set_color(std::uint32_t fg, std::uint32_t bg) {
        prev_fg = current_fg;
        prev_bg = current_bg;
        
        current_fg = fg;
        current_bg = bg;
    }

    void set_color(RgbColor fg, RgbColor bg) {
        set_color(static_cast<std::uint32_t>(fg), static_cast<std::uint32_t>(bg));
    }

    void reset_color() {
        current_fg = prev_fg;
        current_bg = prev_bg;
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
