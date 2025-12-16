#include <kernel/drivers/framebuffer/framebuffer.hpp>
#include "kernel/log/log.hpp"

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

    static kernel::console::driver_config console_driver = {
        .putchar_fn = nullptr,
        .get_color_fn = nullptr,
        .set_color_fn = nullptr,
        .clear_fn = clear_black,
        .put_pixel_fn = put_pixel,
        .get_pixel_fn = get_pixel,
        .get_screen_width_fn = get_screen_width,
        .get_screen_height_fn = get_screen_height,
    };

    kernel::console::driver_config* get_console_driver() {
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
        kernel::log::info("Screen = %dx%d", fb_width, fb_height);
        kernel::log::info("Pitch  = %d", fb_pitch);
        kernel::log::info("BPP    = %d", fb_bpp);
        kernel::log::info("VRAM   = %x", vram);
    }
}
