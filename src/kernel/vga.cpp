#include "vga.hpp"
#include <cstdarg>
#include <cstdint>

static int cursor_x = 0;
static int cursor_y = 0;

static int double_buffer_row = 0;
static int double_buffer_col = 0;

constexpr std::uint16_t vga_entry(const char c, const vga::Color color) {
    return static_cast<std::uint16_t>(c) | static_cast<std::uint16_t>(color) << 8;
}

void vga::initialize() {
    cursor_x = 0;
    cursor_y = 0;

    double_buffer_row = 0;
    double_buffer_col = 0;

    const auto blank = vga_entry(' ', vga::Color::LIGHT_GREY);

    for (int row = 0; row < DOUBLE_BUFFER_ROWS; row++) {
        for (int col = 0; col < DOUBLE_BUFFER_COLS; col++) {
            vga::double_buffer[row][col] = blank;
        }
    }

    write_double_buffer();
}

void vga::clear_screen() {
    for (int y = 0; y < vga::BUFFER_ROWS; y++) {
        for (int x = 0; x < vga::BUFFER_COLS; x++) {
            const int index = y * vga::BUFFER_COLS + x;

            vga::buffer[index] = vga_entry(' ', vga::Color::LIGHT_GREY);
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}

void vga::write_double_buffer() {
    for (int row = 0; row < vga::BUFFER_ROWS; row++) {
        for (int col = 0; col < vga::BUFFER_COLS; col++) {
            const int index = row * vga::BUFFER_COLS + col;

            vga::buffer[index] = vga::double_buffer[double_buffer_row + row][col];
        }
    }
}

void vga::print_int(int n, const Color color) {
    if (n < 0) {
        vga::print_char('-');

        n = -n;
    }

    char buff[32];
    int index = 0;

    while (n > 0) {
        const int digit = n % 10;
        const auto c = static_cast<char>('0' + digit);

        buff[index] = c;
        index++;

        n /= 10;
    }

    index--;

    while (index >= 0) {
        vga::print_char(buff[index--], color);
    }
}

void vga::newline() {
    double_buffer_col = 0;
    double_buffer_row++;
}

void vga::print_char(const char c, const Color color) {
    if (c == '\n') {
        double_buffer_col = 0;
        double_buffer_row++;
    } else {
        vga::double_buffer[double_buffer_row][double_buffer_col] = vga_entry(c, color);
        double_buffer_col++;

        if (double_buffer_col >= vga::DOUBLE_BUFFER_COLS) {
            vga::newline();
        }
    }

    vga::write_double_buffer();
}

void vga::print_str(const char* str, const Color color) {
    while (*str != '\0') {
        vga::print_char(*str++, color);
    }
}

void vga::printf(const char* format, ...) {
    std::va_list args;

    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++;

            switch (*format) {
            case 'd':
                vga::print_int(va_arg(args, int));
                break;
            case 's':
                vga::print_str(va_arg(args, const char*));
                break;
            case '%':
                vga::print_char('%');
            }
        } else {
            vga::print_char(*format);
        }

        format++;
    }
}
