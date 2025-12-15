#include "kernel/kprintf/kprintf.hpp"
#include <cstdarg>
#include <cstdint>
#include <kernel/log/log.hpp>

namespace kernel::log {
    namespace detail {
        void log_with_color(console::color fg, const char* str) {
            const auto saved_color = console::get_color();

            console::set_color(fg, console::color::BLACK);

            kprintf("%s", str);

            // Restore original color
            std::uint8_t saved_fg = saved_color & 0x0F;
            std::uint8_t saved_bg = (saved_color >> 4) & 0x0F;

            console::set_color(static_cast<console::color>(saved_fg),
                               static_cast<console::color>(saved_bg));
        }
        
        void log_with_color(console::color fg,
                            const char* prefix,
                            const char* format,
                            std::va_list args) {
            const auto saved_color = console::get_color();

            console::set_color(fg, console::color::BLACK);

            kprintf("%s", prefix);

            // Restore original color
            std::uint8_t saved_fg = saved_color & 0x0F;
            std::uint8_t saved_bg = (saved_color >> 4) & 0x0F;

            console::set_color(static_cast<console::color>(saved_fg),
                               static_cast<console::color>(saved_bg));

            kvprintf(format, args);
        }
    }

    void info(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::color::LIGHT_CYAN, "[INFO] ", format, args);
        kprintf("\n");
        va_end(args);
    }

    void error(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::color::RED, "[ERROR] ", format, args);
        kprintf("\n");
        va_end(args);
    }

    void warn(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::color::LIGHT_BROWN, "[WARN] ", format, args);
        kprintf("\n");
        va_end(args);
    }

    void init_start(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::color::LIGHT_GREY, "[INIT] ", format, args);
        kprintf("...\n");
        va_end(args);
    }

    void init_end(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::color::LIGHT_GREEN, "[INIT] ", format, args);
        kprintf(" Initialized\n");
        va_end(args);
    }

    void success(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::color::LIGHT_GREEN, "[SUCC] ", format, args);
        kprintf("\n");
        va_end(args);
    }
}
