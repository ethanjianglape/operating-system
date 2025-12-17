#include "vga.hpp"
#include "kernel/console/console.hpp"

#include <cstddef>
#include <cstdint>

namespace i686::drivers::vga {
    static std::size_t cursor_x;
    static std::size_t cursor_y;

    static std::uint8_t current_color;

    static std::uint16_t* vga_buffer = reinterpret_cast<std::uint16_t*>(VGA_BUFFER_ADDR);

    static kernel::console::ConsoleDriver driver {
        .clear = clear,
        .mode = kernel::console::ConsoleMode::TEXT
    };

    inline constexpr std::uint16_t vga_entry(std::uint8_t c, std::uint8_t color) {
        return static_cast<std::uint16_t>(c) | (static_cast<std::uint16_t>(color) << 8);
    }
    
    void init() {
        cursor_x = 0;
        cursor_y = 0;

        clear();
    }

    kernel::console::ConsoleDriver* get_driver() {
        return &driver;
    }

    void set_color(std::uint8_t fg, std::uint8_t bg) {
        current_color = (bg << 4) | (fg & 0x0F);
    }

    std::uint8_t get_color() {
        return current_color;
    }

    void putchar(char c) {
        if (c == '\n') {
            newline();
        } else {
            const std::size_t index = cursor_y * VGA_WIDTH + cursor_x;
            const std::uint16_t entry = vga_entry(c, current_color);

            vga_buffer[index] = entry;

            cursor_x++;

            if (cursor_x >= VGA_WIDTH) {
                newline();
            }
        }
    }

    void newline() {
        cursor_x = 0;
        cursor_y++;

        if (cursor_y >= VGA_HEIGHT) {
            scroll();
        }
    }

    void scroll() {
        for (std::size_t row = 0; row < VGA_HEIGHT - 1; row++) {
            for (std::size_t col = 0; col < VGA_WIDTH; col++) {
                const std::size_t from = (row + 1) * VGA_WIDTH + col;
                const std::size_t to = row * VGA_WIDTH + col;

                vga_buffer[to] = vga_buffer[from];
            }
        }

        const std::uint16_t entry = vga_entry(' ', current_color);

        for (std::size_t col = 0; col < VGA_WIDTH; col++) {
            const std::size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + col;

            vga_buffer[index] = entry;
        }

        cursor_x = 0;
        cursor_y = VGA_HEIGHT - 1;
    }

    void clear() {
        const std::uint16_t entry = vga_entry(' ', current_color);
        
        for (std::size_t row = 0; row < VGA_HEIGHT; row++) {
            for (std::size_t col = 0; col < VGA_WIDTH; col++) {
                const std::size_t index = row * VGA_WIDTH + col;

                vga_buffer[index] = entry;
            }
        }

        cursor_x = 0;
        cursor_y = 0;
    }
}
