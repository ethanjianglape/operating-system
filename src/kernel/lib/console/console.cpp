#include "containers/kstring.hpp"
#include "kernel/log/log.hpp"
#include <kernel/console/console.hpp>
#include <kernel/console/font8x16.hpp>
#include <kernel/timer/timer.hpp>
#include <kernel/drivers/framebuffer/framebuffer.hpp>

#include <cstdint>

namespace kernel::console {
    namespace fb = kernel::drivers::framebuffer;

    static std::uint32_t cursor_x = 0;
    static std::uint32_t cursor_y = 0;

    static std::uint32_t current_fg = static_cast<std::uint32_t>(RgbColor::WHITE);
    static std::uint32_t current_bg = static_cast<std::uint32_t>(RgbColor::BLACK);

    static std::uint32_t prev_fg = current_fg;
    static std::uint32_t prev_bg = current_bg;

    static bool cursor_enabled = true;
    static bool cursor_visible = false;

    std::uint32_t get_pixel_x(int offset = 0) {
        return (cursor_x + offset) * fonts::FONT_WIDTH;
    }

    std::uint32_t get_pixel_y(int offset = 0) {
        return (cursor_y + offset) * fonts::FONT_HEIGHT;
    }

    void blink_handler(std::uintmax_t ticks) {
        
    }

    void init() {
        cursor_x = 0;
        cursor_y = 0;
        enable_cursor();
        draw_cursor();
        kernel::timer::register_handler(blink_handler);
    }

    void enable_cursor() {
        if (!cursor_enabled) {
            cursor_enabled = true;
            draw_cursor();
        }
    }
    
    void disable_cursor() {
        if (cursor_enabled) {
            cursor_enabled = false;
            draw_cursor();
        }
    }

    void show_cursor() { cursor_visible = true; }
    void hide_cursor() { cursor_visible = false; }
    void toggle_cursor() { cursor_visible = !cursor_visible; }

    void draw_cursor() {
        const std::uint32_t px = get_pixel_x();
        const std::uint32_t py = get_pixel_y();

        fb::invert_rec(px, py, fonts::FONT_WIDTH, fonts::FONT_HEIGHT);
    }

    void clear_cursor() {
        const std::uint32_t px = get_pixel_x();
        const std::uint32_t py = get_pixel_y();

        fb::invert_rec(px, py, fonts::FONT_WIDTH, fonts::FONT_HEIGHT);
    }

    void clear_to_eol() {
        const std::uint32_t px = get_pixel_x();
        const std::uint32_t py = get_pixel_y();
        const std::uint32_t width = fb::get_screen_width() - px;

        fb::draw_rec(px, py, width, fonts::FONT_HEIGHT, current_bg);
    }

    void set_cursor_x(std::uint32_t x) {
        set_cursor(x, cursor_y);
    }

    void set_cursor_y(std::uint32_t y) {
        set_cursor(cursor_x, y);
    }

    void set_cursor(std::uint32_t x, std::uint32_t y) {
        if (cursor_x == x && cursor_y == y) {
            return;
        }

        if (cursor_enabled) {
            draw_cursor();            
        }
        
        cursor_x = x;
        cursor_y = y;

        if (cursor_enabled) {
            draw_cursor();            
        }

        const auto px = get_pixel_x(1);
        const auto py = get_pixel_y(1);

        if (py > fb::get_screen_height()) {
            scroll();
        }

        if (px >= fb::get_screen_width()) {
            newline();
        }
    }

    void move_cursor(std::int32_t dx, std::int32_t dy) {
        set_cursor(cursor_x + dx, cursor_y + dy);
    }

    void newline() {
        set_cursor(0, cursor_y + 1);
    }

    void scroll() {
        const auto width = fb::get_screen_width();
        const auto height = fb::get_screen_height();
        
        for (std::uint32_t pixel_y = 0; pixel_y < height - fonts::FONT_HEIGHT; pixel_y++) {
            for (std::uint32_t pixel_x = 0; pixel_x < width; pixel_x++) {
                const auto pixel = fb::get_pixel(pixel_x, pixel_y + fonts::FONT_HEIGHT);
                fb::draw_pixel(pixel_x, pixel_y, pixel);
            }
        }

        cursor_y--;
    }

    void backspace() {
        if (cursor_x == 0) {
            return;
        }

        move_cursor(-1, 0);
    }

    int put(char c) {
        if (c == '\n') {
            newline();
            return 1;
        }

        const std::uint8_t* glyph = fonts::get_glyph(c);

        const std::uint32_t pixel_x = cursor_x * fonts::FONT_WIDTH;
        const std::uint32_t pixel_y = cursor_y * fonts::FONT_HEIGHT;

        move_cursor(1, 0);
        
        for (std::uint8_t y = 0; y < fonts::FONT_HEIGHT; y++) {
            const std::uint8_t byte = glyph[y];

            for (std::uint8_t x = 0; x < fonts::FONT_WIDTH; x++) {
                const std::uint8_t pixel = (byte >> (fonts::FONT_WIDTH - x - 1)) & 1;

                if (pixel == 1) {
                    fb::draw_pixel(pixel_x + x, pixel_y + y, current_fg);
                } else {
                    fb::draw_pixel(pixel_x + x, pixel_y + y, current_bg);
                }
            }
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

    int put(const kernel::kstring& str) {
        int written = 0;

        for (char c : str) {
            written += put(c);
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
        fb::clear_black();
        set_cursor(0, 0);
    }
}
