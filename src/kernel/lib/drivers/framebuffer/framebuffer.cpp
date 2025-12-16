#include "kernel/kprintf/kprintf.hpp"
#include "kernel/log/log.hpp"
#include <kernel/drivers/framebuffer/framebuffer.hpp>

#include <cstdint>

namespace kernel::drivers::framebuffer {
    static std::uint32_t fb_width;
    static std::uint32_t fb_height;
    static std::uint32_t fb_pitch;
    static std::uint8_t fb_bpp;

    static std::uint8_t* vram = nullptr;

    static kernel::console::driver_config console_driver = {
        .putchar_fn = nullptr,
        .get_color_fn = nullptr,
        .set_color_fn = nullptr,
        .clear_fn = clear_black
    };

    kernel::console::driver_config* get_console_driver() {
        return &console_driver;
    }

    inline constexpr std::size_t get_pixel_offset(std::uint32_t x, std::uint32_t y) {
        return (y * fb_pitch) + (x * (fb_bpp / 8));
    }

    void init(const FrameBufferInfo& info) {
        kernel::log::init_start("Framebuffer");
        kernel::log::info("Screen = %dx%d", info.width, info.height);
        kernel::log::info("Pitch  = %d", info.pitch);
        kernel::log::info("BPP    = %d", info.bpp);
        kernel::log::info("VRAM   = %x", info.vram);

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

    void clear_black() {
        clear(RGB_RED);
    }

    void clear(std::uint32_t color) {
        for (std::uint32_t y = 0; y < fb_height; y++) {
            for (std::uint32_t x = 0; x < fb_width; x++) {
                put_pixel(x, y, color);
            }
        }
    }
}
