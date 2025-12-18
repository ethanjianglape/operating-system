#include "kernel/console/console.hpp"
#include <kernel/drivers/framebuffer/framebuffer.hpp>
#include <kernel/log/log.hpp>

#include <cstdint>

namespace kernel::drivers::framebuffer {
    static std::uint32_t fb_width;
    static std::uint32_t fb_height;
    static std::uint32_t fb_pitch;
    static std::uint8_t fb_bpp;

    static std::uint8_t* vram = nullptr;

    static std::uint32_t get_screen_width() {
        return fb_width;
    }

    static std::uint32_t get_screen_height() {
        return fb_height;
    }

    static kernel::console::ConsoleDriver console_driver = {
        .clear = clear_black,
        .put_pixel = put_pixel,
        .get_pixel = get_pixel,
        .get_screen_width = get_screen_width,
        .get_screen_height = get_screen_height,
        .mode = console::ConsoleMode::PIXEL_BUFFER
    };

    kernel::console::ConsoleDriver* get_console_driver() {
        return &console_driver;
    }

    inline constexpr std::size_t get_pixel_offset(std::uint32_t x, std::uint32_t y) {
        return (y * fb_pitch) + (x * (fb_bpp / 8));
    }

    void init(const FrameBufferInfo& info) {
        fb_width = info.width;
        fb_height = info.height;
        fb_pitch = info.pitch;
        fb_bpp = info.bpp;
        vram = info.vram;
    }

    void put_pixel(std::uint32_t x, std::uint32_t y, std::uint32_t color) {
        const auto offset = get_pixel_offset(x, y);
        const auto blue = color & 0xFF;
        const auto green = (color >> 8) & 0xFF;
        const auto red = (color >> 16) & 0xFF;

        vram[offset + 0] = blue;
        vram[offset + 1] = green;
        vram[offset + 2] = red;
    }

    std::uint32_t get_pixel(std::uint32_t x, std::uint32_t y) {
        std::uint32_t pixel = 0x00000000;
        const auto offset = get_pixel_offset(x, y);

        pixel |= vram[offset + 0];
        pixel |= vram[offset + 1] << 8;
        pixel |= vram[offset + 2] << 16;

        return pixel;
    }

    void draw_bitmap(std::uint32_t px,
                     std::uint32_t py,
                     std::uint32_t width,
                     std::uint32_t height,
                     std::uint8_t* bitmap,
                     std::uint32_t fg,
                     std::uint32_t bg) {
        for (std::uint32_t row = 0; row < height; row++) {
            std::uint8_t byte = bitmap[row];

            for (std::uint8_t col = 0; col < width; col++) {
                std::uint8_t pixel = (byte >> (width - col - 1)) & 1;

                if (pixel) {
                    put_pixel(px + col, py + row, fg);
                } else {
                    put_pixel(px + col, py + row, bg);
                }
            }
        }
    }

    void clear_black() {
        clear(RGB_BLACK);
    }

    void clear(std::uint32_t color) {
        for (std::uint32_t y = 0; y < fb_height; y++) {
            for (std::uint32_t x = 0; x < fb_width; x++) {
                put_pixel(x, y, color);
            }
        }
    }

    void log() {
        kernel::log::info("Screen = %dx%dx%d", fb_width, fb_height, fb_bpp);
        kernel::log::info("VRAM   = %x", vram);
    }
}
