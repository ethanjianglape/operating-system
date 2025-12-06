#include <kernel/tty.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static uint16_t* vga_buffer = (uint16_t*) 0xB8000;

static size_t cursor_x;
static size_t cursor_y;

void terminal_initialize(void) {
    cursor_x = 0;
    cursor_y = 0;

    terminal_clear();
}

void terminal_clear(void) {
    const uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    const uint16_t entry = vga_entry(' ', color);

    for (size_t row = 0; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            const size_t index = row * VGA_WIDTH + col;

            vga_buffer[index] = entry;
        }
    }
}

void terminal_scroll(void) {
    for (size_t row = 0; row < VGA_HEIGHT - 1; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            const size_t from = (row + 1) * VGA_WIDTH + col;
            const size_t to = row * VGA_WIDTH + col;

            vga_buffer[to] = vga_buffer[from];
        }
    }

    const uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    const uint16_t entry = vga_entry(' ', color);

    for (size_t col = 0; col < VGA_WIDTH; col++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + col;

        vga_buffer[index] = entry;
    }
}

void terminal_newline(void) {
    cursor_x = 0;
    cursor_y++;

    if (cursor_y >= VGA_HEIGHT) {
        terminal_scroll();
        cursor_y = VGA_HEIGHT - 1;
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
    } else {
        const size_t index = cursor_y * VGA_WIDTH + cursor_x;
        const uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        const uint16_t entry = vga_entry(c, color);

        vga_buffer[index] = entry;

        cursor_x++;

        if (cursor_x >= VGA_WIDTH) {
            terminal_newline();
        }
    }
}

void terminal_puts(const char* str) {
    while (*str != '\0') {
        terminal_putchar(*str++);
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}
