#include <console/console.hpp>
#include <drivers/framebuffer/framebuffer.hpp>
#include <fmt/fmt.hpp>
#include <log/log.hpp>
#include <arch/arch.hpp>

#include <cstdint>

namespace drivers::framebuffer {
    static std::uint64_t fb_width;
    static std::uint64_t fb_height;
    static std::uint64_t fb_pitch;
    static std::uint16_t fb_bpp;

    static std::uint8_t* vram = nullptr;

    std::uint32_t get_screen_width() {
        return fb_width;
    }

    std::uint32_t get_screen_height() {
        return fb_height;
    }

    inline constexpr std::size_t get_pixel_offset(std::uint32_t x, std::uint32_t y) {
        return (y * fb_pitch) + (x * (fb_bpp / 8));
    }

    void init(const FrameBufferInfo& info) {
        log::init_start("Framebuffer");
        log::info("Framebuffer: ", info.width, "x", info.height, "x", info.bpp, " (pitch=", info.pitch, ")");
        log::info("VRAM: ", fmt::hex{info.vram});
        
        fb_width = info.width;
        fb_height = info.height;
        fb_pitch = info.pitch;
        fb_bpp = info.bpp;
        vram = info.vram;

        log::init_end("Framebuffer");
    }

    void draw_pixel(std::uint32_t x, std::uint32_t y, std::uint32_t color) {
        const auto offset = get_pixel_offset(x, y);
        const auto blue = color & 0xFF;
        const auto green = (color >> 8) & 0xFF;
        const auto red = (color >> 16) & 0xFF;

        vram[offset + 0] = blue;
        vram[offset + 1] = green;
        vram[offset + 2] = red;
    }

    void invert_rec(std::uint32_t x, std::uint32_t y, std::uint32_t w, std::uint32_t h) {
        for (std::uint32_t px = x; px < x + w; px++) {
            for (std::uint32_t py = y; py < y + h; py++) {
                const auto color = get_pixel(px, py);
                draw_pixel(px, py, ~color);
            }
        }
    }

    void draw_rec(std::uint32_t x, std::uint32_t y, std::uint32_t w, std::uint32_t h, std::uint32_t color) {
        for (std::uint32_t px = x; px < x + w; px++) {
            for (std::uint32_t py = y; py < y + h; py++) {
                draw_pixel(px, py, color);
            }
        }
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
                draw_pixel(x, y, color);
            }
        }
    }

    void log() {
        log::info("Screen = ", fb_width, "x", fb_height, "x", fb_bpp);
        log::info("VRAM   = ", fmt::hex{vram});
    }
}
