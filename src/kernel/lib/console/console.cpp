#include "containers/kvector.hpp"
#include <containers/kstring.hpp>
#include <cstddef>
#include <log/log.hpp>
#include <console/console.hpp>
#include <console/font8x16.hpp>
#include <timer/timer.hpp>
#include <drivers/framebuffer/framebuffer.hpp>

#include <cstdint>

namespace console {
    namespace fb = drivers::framebuffer;

    static std::uint32_t cursor_col = 0;
    static std::uint32_t cursor_row = 0;

    static std::uint32_t current_fg = static_cast<std::uint32_t>(RgbColor::WHITE);
    static std::uint32_t current_bg = static_cast<std::uint32_t>(RgbColor::BLACK);

    static std::uint32_t prev_fg = current_fg;
    static std::uint32_t prev_bg = current_bg;

    static std::size_t screen_cols;
    static std::size_t screen_rows;

    static std::size_t viewport_offset;

    static bool cursor_enabled = true;

    static kvector<kvector<ConsoleChar>> buffer;
    //static kvector<kvector<bool>> dirty;

    std::uint32_t get_cursor_pixel_x(int offset = 0) {
        return (cursor_col + offset) * fonts::FONT_WIDTH;
    }

    std::uint32_t get_cursor_pixel_y(int offset = 0) {
        return (cursor_row - viewport_offset + offset) * fonts::FONT_HEIGHT;
    }

    bool cursor_within_viewport() {
        return (cursor_row - viewport_offset) < screen_rows;
    }

    void blink_handler(std::uintmax_t ticks) {
    }

    kvector<char> empty_line() {
        return kvector<char>(screen_cols, ' ');
    }

    void init() {
        log::init_start("Console");
        
        cursor_col = 0;
        cursor_row = 0;
        viewport_offset = 0;

        screen_cols = fb::get_screen_width() / fonts::FONT_WIDTH;
        screen_rows = fb::get_screen_height() / fonts::FONT_HEIGHT;

        cursor_enabled = true;

        for (std::size_t row = 0; row < screen_rows; row++) {
            //dirty.push_back(kvector<bool>(screen_cols, false));
        }
        
        log::info("Console size: ", screen_cols, "x", screen_rows, " characters");
        log::info("Console font: ", fonts::FONT_WIDTH, "x", fonts::FONT_HEIGHT, " pixels");
        log::info("Console cursor set to (", cursor_col, ", ", cursor_row, ")");

        log::init_end("Console");
    }

    void enable_cursor() {
        if (!cursor_enabled) {
            cursor_enabled = true;
        }
    }
    
    void disable_cursor() {
        if (cursor_enabled) {
            cursor_enabled = false;
        }
    }

    void draw_cursor() {
        const std::uint32_t px = get_cursor_pixel_x();
        const std::uint32_t py = get_cursor_pixel_y();

        fb::invert_rec(px, py, fonts::FONT_WIDTH, fonts::FONT_HEIGHT);
    }

    void clear_to_eol() {
        for (std::size_t col = cursor_col; col < screen_cols; col++) {
            buffer[cursor_row][col].c = ' ';
            buffer[cursor_row][col].dirty = true;
        }
    }

    void set_cursor_x(std::uint32_t x) {
        set_cursor(x, cursor_row);
    }

    void set_cursor_y(std::uint32_t y) {
        set_cursor(cursor_col, y);
    }

    void ensure_valid_cursor_buffer_pos(std::size_t row, std::size_t col) {
        while (row >= buffer.size()) {
            buffer.push_back({});
        }

        kvector<ConsoleChar>& line = buffer[row];

        while (col >= line.size()) {
            line.push_back(ConsoleChar{
                .c = ' ',
                .fg = current_fg,
                .bg = current_bg,
                .dirty = true
            });
        }
    }

    void set_cursor(std::uint32_t col, std::uint32_t row) {
        if (cursor_col == col && cursor_row == row) {
            return;
        }

        ensure_valid_cursor_buffer_pos(row, col);
        cursor_col = col;
        cursor_row = row;

        if (cursor_col >= screen_cols) {
            newline();
        }

        if (cursor_row - viewport_offset >= screen_rows) {
            scroll();
        }
    }

    void move_cursor(std::int32_t dx, std::int32_t dy) {
        set_cursor(cursor_col + dx, cursor_row + dy);
    }

    void newline() {
        buffer[cursor_row][cursor_col].dirty = true;
        set_cursor(0, cursor_row + 1);
        redraw();
    }

    void scroll_up() {
        if (viewport_offset > 0) {
            viewport_offset--;
            redraw(true);
        }
    }

    void scroll_down() {
        if (viewport_offset < cursor_row) {
            viewport_offset++;
            redraw(true);
        }
    }

    void scroll_into_view() {
        if (cursor_within_viewport()) {
            return;
        }

        viewport_offset = cursor_row;
    }

    void scroll() {
        viewport_offset++;
        redraw(true);
    }

    void backspace() {
        if (cursor_col == 0) {
            return;
        }

        move_cursor(-1, 0);
    }

    void draw_character_at(char c, std::size_t row, std::size_t col, std::uint32_t fg, std::uint32_t bg) {
        const std::uint32_t pixel_x = col * fonts::FONT_WIDTH;
        const std::uint32_t pixel_y = row * fonts::FONT_HEIGHT;

        if (c == ' ') {
            fb::draw_rec(pixel_x, pixel_y, fonts::FONT_WIDTH, fonts::FONT_HEIGHT, bg);
            return;
        }

        const std::uint8_t* glyph = fonts::get_glyph(c);

        for (std::uint8_t y = 0; y < fonts::FONT_HEIGHT; y++) {
            const std::uint8_t byte = glyph[y];

            for (std::uint8_t x = 0; x < fonts::FONT_WIDTH; x++) {
                const std::uint8_t pixel = (byte >> (fonts::FONT_WIDTH - x - 1)) & 1;

                if (pixel == 1) {
                    fb::draw_pixel(pixel_x + x, pixel_y + y, fg);
                } else {
                    fb::draw_pixel(pixel_x + x, pixel_y + y, bg);
                }
            }
        }
    }

    int put(char c) {
        if (c == '\n') {
            newline();
            return 1;
        }

        while (cursor_row >= buffer.size()) {
            buffer.push_back({});
        }

        kvector<ConsoleChar>& line = buffer[cursor_row];

        while (cursor_col >= line.size()) {
            line.push_back(ConsoleChar{
                .c = ' ',
                .fg = current_fg,
                .bg = current_bg,
                .dirty = true
            });
        }

        buffer[cursor_row][cursor_col].c = c;
        buffer[cursor_row][cursor_col].dirty = true;

        move_cursor(1, 0);

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

    int put(const kstring& str) {
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
        set_cursor(0, cursor_row + 1);
        viewport_offset = cursor_row;
        redraw();
   }

    void redraw(bool draw_clean) {
        log::debug("*** redrawing console ***");
        log::debug("* cursor at row=", cursor_row, " col=", cursor_col);
        log::debug("* viewport offset=", viewport_offset);
        log::debug("* num lines=", buffer.size());
        log::debug("*************************");

        if (draw_clean) {
            fb::clear(current_bg);
        }

        for (std::size_t row = viewport_offset; row < buffer.size(); row++) {
            kvector<ConsoleChar>& line = buffer[row];
            
            for (std::size_t col = 0; col < screen_cols; col++){
                ConsoleChar& cc = line[col];
                
                if ((draw_clean || cc.dirty) && row - viewport_offset < screen_rows) {
                    if (col < line.size()) {
                        draw_character_at(cc.c, row - viewport_offset, col, cc.fg, cc.bg);
                    } else {
                        draw_character_at(' ', row - viewport_offset, col, current_fg, current_bg);
                    }

                    cc.dirty = false;
                }
            }
        }

        if (cursor_enabled && cursor_within_viewport()) {
            draw_cursor();
        }
    }
}
