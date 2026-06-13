#include "arch/x64/interrupts/irq.hpp"
#include "memory/memory.hpp"
#include "timer/timer.hpp"
#include <arch.hpp>
#include <console/console.hpp>
#include <crt/crt.h>
#include <fmt/fmt.hpp>
#include <framebuffer/framebuffer.hpp>
#include <log/log.hpp>

#include <cstdint>

namespace framebuffer {
static std::uint64_t fb_width;
static std::uint64_t fb_height;
static std::uint64_t fb_num_pixels;
static std::uint64_t fb_pitch;
static std::uint16_t fb_bpp;

static std::uint8_t* vram = nullptr;
static std::uint8_t* vram_end = nullptr;
static std::uint64_t vram_size;

static std::uint8_t* vram_buff = nullptr;
static std::uint8_t* vram_buff_end = nullptr;

static bool needs_redraw = false;

std::uint32_t get_screen_width()
{
    return fb_width;
}

std::uint32_t get_screen_height()
{
    return fb_height;
}

inline constexpr std::size_t get_pixel_offset(std::uint32_t x, std::uint32_t y)
{
    return (y * fb_pitch) + (x * (fb_bpp / 8));
}

void draw_timer(std::uintmax_t ticks, arch::irq::InterruptFrame*)
{
    constexpr int target_fps = 30;
    constexpr int ms_per_frame = 1000 / target_fps;

    if (ticks % ms_per_frame == 0 && needs_redraw) {
        buffer();
        needs_redraw = false;
    }
}

void init(const FrameBufferInfo& info)
{
    log::init_start("Framebuffer");

    fb_width = info.width;
    fb_height = info.height;
    fb_num_pixels = fb_width * fb_height;
    fb_pitch = info.pitch;
    fb_bpp = info.bpp;

    vram_size = fb_num_pixels * (fb_bpp / 8);
    vram = info.vram;
    vram_end = vram + vram_size;

    vram_buff = kalloc<std::uint8_t>(vram_size);
    vram_buff_end = vram_buff + vram_size;

    timer::register_handler(draw_timer);

    log::info("Framebuffer: ", fb_width, "x", fb_height, " @ ", fb_bpp, " bpp (pitch=", fb_pitch, ")");
    log::info("Framebuffer # pixels: ", fb_num_pixels);
    log::info("VRAM: ", fmt::hex{vram});
    log::info("VRAM Back Buffer = ", fmt::hex{vram_buff});
    log::info("VRAME Back Buffer End = ", fmt::hex{vram_buff_end});
    log::info("VRAM Size: ", vram_size);

    log::init_end("Framebuffer");
}

void draw_pixel(std::uint32_t x,
    std::uint32_t y,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue)
{
    const auto offset = get_pixel_offset(x, y);

    vram_buff[offset + RGB_OFFB] = blue;
    vram_buff[offset + RGB_OFFG] = green;
    vram_buff[offset + RGB_OFFR] = red;

    needs_redraw = true;
}

void draw_pixel(std::uint32_t x, std::uint32_t y, std::uint32_t color)
{
    const auto blue = color & 0xFF;
    const auto green = (color >> 8) & 0xFF;
    const auto red = (color >> 16) & 0xFF;

    draw_pixel(x, y, red, green, blue);
}

void buffer()
{
    memcpy(vram, static_cast<const std::uint8_t*>(vram_buff), vram_size);
}

void invert_rec(std::uint32_t x, std::uint32_t y, std::uint32_t w, std::uint32_t h)
{
    for (std::uint32_t px = x; px < x + w; px++) {
        for (std::uint32_t py = y; py < y + h; py++) {
            const auto color = get_pixel(px, py);
            draw_pixel(px, py, ~color);
        }
    }
}

void draw_rec(std::uint32_t x, std::uint32_t y, std::uint32_t w, std::uint32_t h, std::uint32_t color)
{
    const auto blue = color & 0xFF;
    const auto green = (color >> 8) & 0xFF;
    const auto red = (color >> 16) & 0xFF;

    for (std::uint32_t px = x; px < x + w; px++) {
        for (std::uint32_t py = y; py < y + h; py++) {
            draw_pixel(px, py, red, green, blue);
        }
    }
}

std::uint32_t get_pixel(std::uint32_t x, std::uint32_t y)
{
    std::uint32_t pixel = 0x00000000;
    const auto offset = get_pixel_offset(x, y);

    pixel |= vram_buff[offset + RGB_OFFB];
    pixel |= vram_buff[offset + RGB_OFFG] << 8;
    pixel |= vram_buff[offset + RGB_OFFR] << 16;

    return pixel;
}

void clear_black()
{
    clear(RGB_BLACK);
}

void clear(std::uint32_t color)
{
    auto* start = reinterpret_cast<std::uint32_t*>(vram_buff);
    auto* end = reinterpret_cast<std::uint32_t*>(vram_buff_end);

    while (start != end) {
        *start = color;
        start++;
    }
}

void log()
{
    log::info("Screen = ", fb_width, "x", fb_height, "x", fb_bpp);
    log::info("VRAM   = ", fmt::hex{vram});
    log::info("VRAM Back Buffer = ", fmt::hex{vram_buff});
}
}
