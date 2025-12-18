#pragma once

#include <kernel/console/console.hpp>
#include <kernel/kprintf/kprintf.hpp>

#include <utility>

namespace kernel::log {
    template <typename... Ts>
    void log_with_color(console::RgbColor fg,
                        console::RgbColor bg,
                        const char* prefix,
                        const char* format,
                        Ts... args) {
        console::set_color(fg, bg);
        kprintf(prefix);
        console::reset_color();
        kprintf(format, std::forward<Ts>(args)...);
    }
    
    template <typename... Ts>
    void info(const char* format, Ts... args) {
        log_with_color(console::RgbColor::CYAN,
                       console::RgbColor::BLACK,
                       "[INFO] ",
                       format,
                       std::forward<Ts>(args)...);
        kprintf("\n");
    }

    template <typename... Ts>
    void init_start(const char* format, Ts... args) {
        log_with_color(console::RgbColor::WHITE,
                       console::RgbColor::BLACK,
                       "[INIT] ",
                       format,
                       std::forward<Ts>(args)...);
        kprintf(" initializing...\n");
    }

    template <typename... Ts>
    void init_end(const char* format, Ts... args) {
        log_with_color(console::RgbColor::GREEN,
                       console::RgbColor::BLACK,
                       "[INIT] ",
                       format,
                       std::forward<Ts>(args)...);
        kprintf(" initialized!\n");
    }

    template <typename... Ts>
    void warn(const char* format, Ts... args) {
        log_with_color(console::RgbColor::YELLOW,
                       console::RgbColor::BLACK,
                       "[WARN] ",
                       format,
                       std::forward<Ts>(args)...);
        kprintf("\n");
    }

    template <typename... Ts>
    void error(const char* format, Ts... args) {
        log_with_color(console::RgbColor::RED,
                       console::RgbColor::BLACK,
                       "[ERROR] ",
                       format,
                       std::forward<Ts>(args)...);
        kprintf("\n");
    }

    template <typename... Ts>
    void success(const char* format, Ts... args) {
        log_with_color(console::RgbColor::GREEN,
                       console::RgbColor::BLACK,
                       "[SUCC] ",
                       format,
                       std::forward<Ts>(args)...);
        kprintf("\n");
    }

    template <typename... Ts>
    void debug(const char* format, Ts... args) {
        log_with_color(console::RgbColor::MAGENTA,
                       console::RgbColor::BLACK,
                       "[DEBUG] ",
                       format,
                       std::forward<Ts>(args)...);
        kprintf("\n");
    }
}
